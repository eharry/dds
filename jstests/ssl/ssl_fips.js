// Test mongod start with FIPS mode enabled
var port = allocatePort();
var md = MongoRunner.runMongod({
    port: port,
    sslMode: "requireSSL",
    sslPEMKeyFile: "jstests/libs/server.pem",
    sslCAFile: "jstests/libs/ca.pem",
    sslFIPSMode: ""
});

var mongo = runMongoProgram("mongo",
                            "--port",
                            port,
                            "--ssl",
                            "--sslAllowInvalidCertificates",
                            "--sslPEMKeyFile",
                            "jstests/libs/client.pem",
                            "--sslFIPSMode",
                            "--eval",
                            ";");

// if mongo shell didn't start/connect properly
if (mongo != 0) {
    print("mongod failed to start, checking for FIPS support");
    mongoOutput = rawMongoProgramOutput();
    assert(mongoOutput.match(/this version of mongodb was not compiled with FIPS support/) ||
           mongoOutput.match(/FIPS modes is not enabled on the operating system/) ||
           mongoOutput.match(/FIPS_mode_set:fips mode not supported/));
} else {
    // verify that auth works, SERVER-18051
    md.getDB("admin").createUser(
        {user: "admin", pwd: "Password@a1b", roles: ["root"], "passwordDigestor": "server"});
    assert(md.getDB("admin").auth("admin", "Password@a1b"), "auth failed");

    // kill mongod
    MongoRunner.stopMongod(md);
}
