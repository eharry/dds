// Test that a user is not allowed to getMore a cursor they did not create, and that such a failed
// getMore will leave the cursor unaffected, so that a subsequent getMore by the original author
// will work.
// @tags: [requires_sharding]

(function() {
    // TODO: Remove 'shardAsReplicaSet: false' when SERVER-32672 is fixed.
    const st = new ShardingTest(
        {shards: 2, config: 1, other: {keyFile: "jstests/libs/key1", shardAsReplicaSet: false}});
    const kDBName = "test";
    const adminDB = st.s.getDB('admin');
    const testDB = st.s.getDB(kDBName);

    jsTest.authenticate(st.shard0);

    const adminUser = {db: "admin", username: "foo", password: "Password@a1b"};
    const userA = {db: "test", username: "a", password: "Password@a1b"};
    const userB = {db: "test", username: "b", password: "Password@a1b"};

    function login(userObj) {
        st.s.getDB(userObj.db).auth(userObj.username, userObj.password);
    }

    function logout(userObj) {
        st.s.getDB(userObj.db).runCommand({logout: 1});
    }

    adminDB.createUser({
        user: adminUser.username,
        pwd: adminUser.password,
        roles: jsTest.adminUserRoles, "passwordDigestor": "server"
    });

    login(adminUser);

    let coll = testDB.security_501;
    coll.drop();

    for (let i = 0; i < 100; i++) {
        assert.writeOK(coll.insert({_id: i}));
    }

    // Create our two users.
    for (let user of[userA, userB]) {
        testDB.createUser({
            user: user.username,
            pwd: user.password,
            roles: [{role: "readWriteAnyDatabase", db: "admin"}], "passwordDigestor": "server"
        });
    }
    logout(adminUser);

    // As userA, run a find and get a cursor.
    login(userA);
    const cursorID =
        assert.commandWorked(testDB.runCommand({find: coll.getName(), batchSize: 2})).cursor.id;
    logout(userA);

    // As userB, attempt to getMore the cursor ID.
    login(userB);
    assert.commandFailed(testDB.runCommand({getMore: cursorID, collection: coll.getName()}));
    logout(userB);

    // As user A again, try to getMore the cursor.
    login(userA);
    assert.commandWorked(testDB.runCommand({getMore: cursorID, collection: coll.getName()}));
    logout(userA);

    st.stop();
})();
