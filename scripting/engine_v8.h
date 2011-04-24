//engine_v8.h

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <vector>
#include "engine.h"
#include <v8.h>

using namespace v8;

namespace mongo {

    class V8ScriptEngine;
    class V8Scope;

    typedef Handle< Value > (*v8Function) ( V8Scope* scope, const v8::Arguments& args );

    // Preemption is going to be allowed for the v8 mutex, and some of our v8
    // usage is not preemption safe.  So we are using an additional mutex that
    // will not be preempted.  The V8Lock should be used in place of v8::Locker
    // except in certain special cases involving interrupts.
    namespace v8Locks {
        // the implementations are quite simple - objects must be destroyed in
        // reverse of the order created, and should not be shared between threads
        struct RecursiveLock {
            RecursiveLock();
            ~RecursiveLock();
            bool _unlock;
        };
        struct RecursiveUnlock {
            RecursiveUnlock();
            ~RecursiveUnlock();
            bool _lock;
        };
    } // namespace v8Locks
    class V8Lock {
        v8Locks::RecursiveLock _noPreemptionLock;
        v8::Locker _preemptionLock;
    };
    struct V8Unlock {
        v8::Unlocker _preemptionUnlock;
        v8Locks::RecursiveUnlock _noPreemptionUnlock;
    };

    class V8Scope : public Scope {
    public:

        V8Scope( V8ScriptEngine * engine );
        ~V8Scope();

        virtual void reset();
        virtual void init( const BSONObj * data );

        virtual void localConnect( const char * dbName );
        virtual void externalSetup();

        v8::Handle<v8::Value> get( const char * field ); // caller must create context and handle scopes
        virtual double getNumber( const char *field );
        virtual int getNumberInt( const char *field );
        virtual long long getNumberLongLong( const char *field );
        virtual string getString( const char *field );
        virtual bool getBoolean( const char *field );
        virtual BSONObj getObject( const char *field );
        Handle<v8::Object> getGlobalObject() { return _global; };

        virtual int type( const char *field );

        virtual void setNumber( const char *field , double val );
        virtual void setString( const char *field , const char * val );
        virtual void setBoolean( const char *field , bool val );
        virtual void setElement( const char *field , const BSONElement& e );
        virtual void setObject( const char *field , const BSONObj& obj , bool readOnly);
        virtual void setThis( const BSONObj * obj );

        virtual void rename( const char * from , const char * to );

        virtual ScriptingFunction _createFunction( const char * code );
        Local< v8::Function > __createFunction( const char * code );
        virtual int invoke( ScriptingFunction func , const BSONObj& args, int timeoutMs = 0 , bool ignoreReturn = false );
        virtual bool exec( const StringData& code , const string& name , bool printResult , bool reportError , bool assertOnError, int timeoutMs );
        virtual string getError() { return _error; }

        virtual void injectNative( const char *field, NativeFunction func );
        void injectNative( const char *field, NativeFunction func, Handle<v8::Object>& obj );
        Handle<v8::Function> injectV8Function( const char *field, v8Function func );
        Handle<v8::Function> injectV8Function( const char *field, v8Function func, Handle<v8::Object>& obj );
        Handle<v8::Function> injectV8Function( const char *field, v8Function func, Handle<v8::Template>& t );
        Handle<v8::FunctionTemplate> createV8Function( v8Function func );

        void gc();

        Handle< Context > context() const { return _context; }

        v8::Local<v8::Object> mongoToV8( const mongo::BSONObj & m , bool array = 0 , bool readOnly = false );
        mongo::BSONObj v8ToMongo( v8::Handle<v8::Object> o , int depth = 0 );

        void v8ToMongoElement( BSONObjBuilder & b , v8::Handle<v8::String> name ,
                               const string sname , v8::Handle<v8::Value> value , int depth = 0 );
        v8::Handle<v8::Value> mongoToV8Element( const BSONElement &f );

        v8::Function * getNamedCons( const char * name );
        v8::Function * getObjectIdCons();
        Local< v8::Value > newId( const OID &id );

        Persistent<v8::String> V8STR_CONN;
        Persistent<v8::String> V8STR_ID;
        Persistent<v8::String> V8STR_LENGTH;
        Persistent<v8::String> V8STR_ISOBJECTID;
        Persistent<v8::String> V8STR_NATIVE_FUNC;
        Persistent<v8::String> V8STR_V8_FUNC;
        Persistent<v8::String> V8STR_RETURN;
        Persistent<v8::String> V8STR_ARGS;
        Persistent<v8::String> V8STR_T;
        Persistent<v8::String> V8STR_I;
        Persistent<v8::String> V8STR_EMPTY;
        Persistent<v8::String> V8STR_MINKEY;
        Persistent<v8::String> V8STR_MAXKEY;
        Persistent<v8::String> V8STR_NUMBERLONG;
        Persistent<v8::String> V8STR_DBPTR;
        Persistent<v8::String> V8STR_BINDATA;
        Persistent<v8::String> V8STR_WRAPPER;

    private:
        void _startCall();

        static Handle< Value > nativeCallback( V8Scope* scope, const Arguments &args );
        static v8::Handle< v8::Value > v8Callback( const v8::Arguments &args );
        static Handle< Value > load( V8Scope* scope, const Arguments &args );
        static Handle< Value > Print(V8Scope* scope, const v8::Arguments& args);
        static Handle< Value > Version(V8Scope* scope, const v8::Arguments& args);
        static Handle< Value > GCV8(V8Scope* scope, const v8::Arguments& args);


        V8ScriptEngine * _engine;

        Persistent<Context> _context;
        Persistent<v8::Object> _global;

        string _error;
        vector< Persistent<Value> > _funcs;
        v8::Persistent<v8::Object> _this;

        v8::Persistent<v8::Function> _wrapper;

        enum ConnectState { NOT , LOCAL , EXTERNAL };
        ConnectState _connectState;
    };

    class V8ScriptEngine : public ScriptEngine {
    public:
        V8ScriptEngine();
        virtual ~V8ScriptEngine();

        virtual Scope * createScope() { return new V8Scope( this ); }

        virtual void runTest() {}

        bool utf8Ok() const { return true; }

        class V8UnlockForClient : public Unlocker {
            V8Unlock u_;
        };

        virtual auto_ptr<Unlocker> newThreadUnlocker() { return auto_ptr< Unlocker >( new V8UnlockForClient ); }

        virtual void interrupt( unsigned opSpec );
        virtual void interruptAll();

    private:
        friend class V8Scope;
    };

    extern ScriptEngine * globalScriptEngine;
    extern map< unsigned, int > __interruptSpecToThreadId;

}
