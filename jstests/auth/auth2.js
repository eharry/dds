// test read/write permissions

m = MongoRunner.runMongod({auth: "", bind_ip: "127.0.0.1", smallfiles: ""});
db = m.getDB("admin");

// These statements throw because the localhost exception does not allow
// these operations: it only allows the creation of the first admin user
// and necessary setup operations.
assert.throws(function() {
    db.users.count();
});
assert.throws(function() {
    db.shutdownServer();
});

db.createUser({user: "admin", pwd: "Password@a1b", roles: ["root"], "passwordDigestor": "server"});

// These statements throw because we have a user but have not authenticated
// as that user.
assert.throws(function() {
    db.users.count();
});
assert.throws(function() {
    db.shutdownServer();
});

db.auth("admin", "Password@a1b");

users = db.getCollection("system.users");
assert.eq(1, users.count());

db.shutdownServer();
waitProgram(m.pid);
