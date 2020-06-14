var conn1 = MongoRunner.runMongod(
    {auth: "", smallfiles: "", setParameter: "enableLocalhostAuthBypass=true"});
var conn2 = MongoRunner.runMongod(
    {auth: "", smallfiles: "", setParameter: "enableLocalhostAuthBypass=false"});

// Should fail because of localhost exception narrowed (SERVER-12621).
assert.writeError(conn1.getDB("test").foo.insert({a: 1}));
assert.throws(function() {
    conn2.getDB("test").foo.findOne();
});

// Should succeed due to localhost exception.
conn1.getDB("admin").createUser(
    {user: "admin", pwd: "Password@a1b", roles: ["root"], "passwordDigestor": "server"});

conn1.getDB("admin").auth("admin", "Password@a1b");
conn1.getDB("test").foo.insert({a: 1});

conn1.getDB("admin").dropAllUsers();
conn1.getDB("admin").logout();

assert.throws(function() {
    conn2.getDB("test").foo.findOne();
});

// Should fail since localhost exception is disabled
assert.throws(function() {
    conn2.getDB("admin").createUser(
        {user: "admin", pwd: "Password@a1b", roles: ["root"], "passwordDigestor": "server"});
});

print("SUCCESS Completed disable_localhost_bypass.js");
MongoRunner.stopMongod(conn1, null, {user: "admin", pwd: "Password@a1b"});
MongoRunner.stopMongod(conn2, null, {user: "admin", pwd: "Password@a1b"});
