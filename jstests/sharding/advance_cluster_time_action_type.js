/**
 * Test a role with an advanceClusterTime action type.
 */

(function() {
    "use strict";

    // TODO SERVER-35447: Multiple users cannot be authenticated on one connection within a session.
    TestData.disableImplicitSessions = true;

    // TODO: Remove 'shardAsReplicaSet: false' when SERVER-32672 is fixed.
    let st = new ShardingTest({
        mongos: 1,
        config: 1,
        shards: 1,
        keyFile: 'jstests/libs/key1',
        mongosWaitsForKeys: true,
        other: {shardAsReplicaSet: false}
    });

    let adminDB = st.s.getDB('admin');

    assert.commandWorked(adminDB.runCommand(
        {createUser: "admin", pwd: "Password@a1b", roles: ["root"], "digestPassword": true}));
    assert.eq(1, adminDB.auth("admin", "Password@a1b"));

    assert.commandWorked(adminDB.runCommand({
        createRole: "advanceClusterTimeRole",
        privileges: [{resource: {cluster: true}, actions: ["advanceClusterTime"]}],
        roles: []
    }));

    let testDB = adminDB.getSiblingDB("testDB");

    assert.commandWorked(testDB.runCommand({
        createUser: 'NotTrusted',
        pwd: 'Password@a1b',
        roles: ['readWrite'], "digestPassword": true
    }));
    assert.commandWorked(testDB.runCommand({
        createUser: 'Trusted',
        pwd: 'Password@a1b',
        roles: [{role: 'advanceClusterTimeRole', db: 'admin'}, 'readWrite'], "digestPassword": true
    }));
    assert.eq(1, testDB.auth("NotTrusted", "Password@a1b"));

    let res = testDB.runCommand({insert: "foo", documents: [{_id: 0}]});
    assert.commandWorked(res);

    let clusterTime = Object.assign({}, res.$clusterTime);
    let clusterTimeTS = new Timestamp(clusterTime.clusterTime.getTime() + 1000, 0);
    clusterTime.clusterTime = clusterTimeTS;

    const cmdObj = {find: "foo", limit: 1, singleBatch: true, $clusterTime: clusterTime};
    jsTestLog("running NonTrusted. command: " + tojson(cmdObj));
    res = testDB.runCommand(cmdObj);
    assert.commandFailed(res, "Command request was: " + tojsononeline(cmdObj));

    assert.eq(1, testDB.auth("Trusted", "Password@a1b"));
    jsTestLog("running Trusted. command: " + tojson(cmdObj));
    res = testDB.runCommand(cmdObj);
    assert.commandWorked(res, "Command request was: " + tojsononeline(cmdObj));

    testDB.logout();

    st.stop();
})();
