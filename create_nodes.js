var mongodb = require("mongodb"),
mongoserver = new mongodb.Server("192.168.1.121", 27017),
db_connector = new mongodb.Db("squares", mongoserver, {w: 1});

var neo4j = require('neo4j');
var neo_db = new neo4j.GraphDatabase('http://localhost:7474');

var nodelist = [];

db_connector.open(function(err, db){
    if (err) {
        console.log(err);
        throw err;
    }

    var coll = new mongodb.Collection(db, 'squares');
    coll.find({}).toArray(function(err, document) {
        for (var i=0; i<document.length; i++) {
            nodelist[document[i].a] = 1;
            nodelist[document[i].b] = 1;
            nodelist[document[i].c] = 1;
            nodelist[document[i].d] = 1;
        }
        for (var j=0; j<80000000; j++) {
            if (nodelist[j] == 1) {
                node = neo_db.createNode({val: j});
                node.save(function (err, node) {
                    if (err) {
                        console.err('Error saving new node ' + j + ' to database:', err);
                    }
                });
            }
        }
    db.close();
    });
});
