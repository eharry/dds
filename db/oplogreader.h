/** @file oplogreader.h */

#pragma once

#include "../client/dbclient.h"
#include "../client/constants.h"

namespace mongo {

    /* started abstracting out the querying of the primary/master's oplog 
       still fairly awkward but a start.
    */
    class OplogReader {
        auto_ptr<DBClientConnection> _conn;
        auto_ptr<DBClientCursor> cursor;
    public:

        void reset() {
            cursor.reset();
        }
        void resetConnection() {
            cursor.reset();
            _conn.reset();
        }
        DBClientConnection* conn() { return _conn.get(); }
        BSONObj findOne(const char *ns, Query& q) { 
            return conn()->findOne(ns, q);
        }

        BSONObj getLastOp(const char *ns) { 
            return findOne(ns, Query().sort( BSON( "$natural" << -1 ) ));
        }

        /* ok to call if already connected */
        bool connect(string hostname);

        void getReady() {
            if( cursor.get() && cursor->isDead() ) { 
                log() << "repl: old cursor isDead, initiating a new one" << endl;
                reset();
            }
        }

        bool haveCursor() { return cursor.get() != 0; }

        void tailingQuery(const char *ns, const BSONObj& query) { 
            assert( !haveCursor() );
            log(2) << "repl: " << ns << ".find(" << query.toString() << ')' << endl;
            cursor = _conn->query( ns, query, 0, 0, 0, 
                                  QueryOption_CursorTailable | QueryOption_SlaveOk | QueryOption_OplogReplay |
                                  QueryOption_AwaitData
                                  );
        }

        void tailingQueryGTE(const char *ns, OpTime t) {
            BSONObjBuilder q;
            q.appendDate("$gte", t.asDate());
            BSONObjBuilder query;
            query.append("ts", q.done());
            tailingQuery(ns, query.done());
        }

        bool more() { 
            assert( cursor.get() );
            return cursor->more();
        }

        /* old mongod's can't do the await flag... */
        bool awaitCapable() { 
            return cursor->hasResultFlag(ResultFlag_AwaitCapable);
        }

        void peek(vector<BSONObj>& v, int n) { 
            if( cursor.get() )
                cursor->peek(v,n);
        }

        BSONObj nextSafe() { return cursor->nextSafe(); }

        BSONObj next() { 
            return cursor->next();
        }

        void putBack(BSONObj op) { 
            cursor->putBack(op);
        }
    };
    
}
