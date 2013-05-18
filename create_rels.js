var mongodb = require("mongodb"),
    mongoserver = new mongodb.Server("192.168.1.121", 27017),
    db_connector = new mongodb.Db("squares", mongoserver, {w: 1});

var neo4j = require('neo4j');
var neo_db = new neo4j.GraphDatabase('http://localhost:7474');

db_connector.open(function(err, db){
    if (err) {
        console.log(err);
        throw err;
    }

    var coll = new mongodb.Collection(db, 'squares');
    coll.find({}).sort( { $d: 1 } ).toArray(function(err, document) {
        for (var i=0; i<document.length; i++) {
            var query = [
            'start a = node:node_auto_index(val={firstEl}), b = node:node_auto_index(val={secondEl}), c = node:node_auto_index(val={thirdEl}), d = node:node_auto_index(val={fourthEl})',
            'create unique a-[r:SUM_SQ]-b',
            'create unique a-[r2:SUM_SQ]-c',
            'create unique a-[r3:SUM_SQ]-d',
            'create unique b-[r4:SUM_SQ]-c',
            'create unique b-[r5:SUM_SQ]-d',
            'create unique c-[r6:SUM_SQ]-d'
            ].join('\n');
            var params = {
                firstEl: document[i].a,
                secondEl: document[i].b,
                thirdEl: document[i].c,
                fourthEl: document[i].d
            };
            console.log(document[i]);
            neo_db.query(query, params, function (err, results) {
                if (err) throw err;
                console.log("done\n");
            });
        }
        //console.log(document);
        db.close();
    });

});
