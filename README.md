# Badanie relacji sumy 2 liczb będącej kwadratem innej liczby

## Problem

Istnieje pewien dotąd nie rozwiązany problem, którym zajmują się pewne osoby na Uniwersytecie Gdańskim:
Określmy graf w którym węzłami są liczby naturalne, węzły łączymy, gdy suma ich wartości jest kwadratem jakiejś liczby. Jaką najmniejszą liczbą kolorów da się pokolorować wierzchołki tego grafu, aby krawędź nie łączyła dwóch wierzchołków tego samego koloru? Czy będzie to jakaś skończona liczba? Czy może połączeń wraz ze zwiększaniem zakresu poszukiwań zawsze będzie dochodziło i wymagana będzie niekończona ilość kolorów?

W ujęciu algorytmicznym można podejść do tego w następujący sposób:
Szukamy największych podgrafów pełnych - tzn par, trójek, czwórek, itd... liczb które wszystkie są ze sobą w relacji. W takim podgrafie trzeba będzie zużyć tyle kolorów ile wynosi stopień tego podgrafu.

## Szukanie podgrafów pełnych

Spodziewając się dużej ilości podgrafów pełnych z coraz to większym stopniem szybko napisałem prosty program sprawdzający relacje na początku między trójkami, czwórkami i piątkami. Oczywiście w miarę zwiększania podgrafu pełnego czas sprawdzania rośnie wykładniczo, więc trzeba mocno ograniczać zakres poszukiwań. Okazało się że o ile czwórek można znaleźć w miarę szybko sporo, to piątek nigdzie nie widać...

Zatem postanowiłem zatrudnić więcej maszyn do pracy. Efektem pracy był programik w C uruchomiony na paru komputerach, który szuka czwórek i piątek w zadanym zakresie, a wyniki zapisuje do bazy danych mongo postawionej na komputerze w sieci lokalnej.

Program w kilku procesach iteruje po zadanych zakresach zapisując wyniki:
```c
for (a = i; a < SIZE1; a+=FORKS){
    for (b = a+1; b < SIZE2; b++) {
        if (is_square(a+b)==1 )
            for (c = b+1; c < SIZE3; c++) {
                if (is_square(a+c)==1 &&  is_square(b+c)==1 ) {
                    for (d = c+1; d < SIZE4; d++) {
                     if ((is_square(d+b)==1) && (is_square(a+d)==1) && is_square(c+d)==1){
                        tab[0]=a; tab[1]=b; tab[2]=c; tab[3]=d;
                        if(saveToDB(4, tab) == -1){
                            if(is_square(a) && is_square(b) && is_square(c) && is_square(d))
                                fprintf(stderr, ">>>>>4sq<<<<< %10ld\t%10ld\t%10ld\t%10ld\n",a,b,c,d);
                            else
                                fprintf(stderr, ">>>>>>4<<<<<< %10ld\t%10ld\t%10ld\t%10ld\n",a,b,c,d);
                        }
                        for (e = d+1; e < SIZE5; e++) {
                            if (is_square(a+e)==1 && (is_square(e+b)==1) && (is_square(e+c)==1) && is_square(e+d)==1){
                                tab[4]=e;
                                if(saveToDB(5, tab) == -1){
                                    printf(">>>>>>5<<<<<< %10ld\t%10ld\t%10ld\t%10ld\t%10ld\n",a,b,c,d, e);
                                }
                            }
                        }
                    }
                }

            }
        }
    }
}
```
Przed zapisem należy oczywiście sprawdzić czy nie istnieje już duplikat:
```c
int checkDup(int count, long *tab) {
    bson query[1];
    mongo_cursor cursor[1];
    bson_init( query );
    for(int i=0; i< count; i++){
        bson_append_int( query, vars[i], tab[i] );
    }
    bson_finish( query );
    mongo_cursor_init( cursor, conn, COLLPATH );
    mongo_cursor_set_query( cursor, query );
    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        bson_iterator iterator[1];
        if ( bson_find( iterator, mongo_cursor_bson( cursor ), "a" )) {
            bson_destroy( query );
            mongo_cursor_destroy( cursor );
            return 1;
        }
    }

    bson_destroy( query );
    mongo_cursor_destroy( cursor );
    return 0;
}
```
Jeżeli nie to zapisuje znaleziony podgraf:
```c
int saveToDB(int count, long *tab){
    if(checkDup(count, tab) == 1)
        return 0;

    bson b[1];

    bson_init( b );
    for(int i=0; i< count; i++){
        bson_append_int( b, vars[i], tab[i] );
    }
    bson_finish( b );

    if( mongo_insert(conn, COLLPATH, b, NULL) == MONGO_ERROR) {
        if(conn->err == MONGO_IO_ERROR ){
            mongo_reconnect( conn );
            if( mongo_insert( conn, COLLPATH, b, NULL) == MONGO_ERROR && conn->err == MONGO_IO_ERROR ){
                bson_destroy( b );
                return -1;
            }
        }
    }

    bson_destroy( b );
    return 1;
}
```

[Pełny kod](/search_fulls.c)

## Wyniki szukania

Efektem było znalezienie 25893 podgrafów pełnych stopnia 4 i 0!! podgrafów pełnych stopnia 5.
Wyniki zostały zapisane w postaci:
```javascrpit
{
    "_id" : ObjectId("518bd279342b2f1912d4155f"),
    "a" : 2,
    "b" : 167,
    "c" : 674,
    "d" : 6722
}
{
    "_id" : ObjectId("518bd279342b2f1912d41560"),
    "a" : 2,
    "b" : 359,
    "c" : 482,
    "d" : 3362
}
```
```
> db.squares.find().count()
25893
```
Brak podgrafów pełnych stopnia 5 rodził pytania, czy przypadkiem nie wystarczą 4 kolory aby pokolorować ten graf...
Oczywiście nie tylko pełne podgrafy stopnia 5 mogłyby wymusić więcej niż 4 kolory, ale w takim wypadku trzebaby analizować otrzymany graf na inne sposoby. Oczywiście można odpytywać bazę danych sprytnymi agregacjami, lecz postanowiłem spróbować ułatwić pracę prezentując otrzymane dane jako... graf.

## Przenoszenie danych

Aby to osiągnąć postanowiłem przenieść wyniki szukania do grafowej bazy danych Neo4j. Na początek, aby uniknąć problemów z tworzeniem relacji na nieistniejących węzłach postanowiłem stworzyć węzły z wszystkimi wartościami z bazy danych. Zrealizował to krótki programik w node/javascript.

```javascript
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
```
[Pełny kod](/create_nodes.js)

Po kilkudziesięciu minutach podczas których procesor próbował naśladować serce tworząc wykres trochę podobny do wykresu ECG podczas częstoskurczu komorowego ;)...
![Prawie wykres ECG](/images/cpu.png)
![Częstoskurcz](/images/czestoskurcz.jpg)

...węzły grafu zostały zaimportowane i panel webadmin radośnie oznajmił, że baza zawiera 51179 węzłów (ale już wyświetlał je wszystkie bez większego entuzjazmu z uwagi na ich ilość).

Oczywiście teraz należało dodać relacje pomiędzy tymi węzłami. API neo4j dla nodejs nie posiada metod tworzenia relacji więc utworzymy je za pomocą zapytania Cypher. Dodatkowo pobrane dane z mongo posortujemy wzgledem ostatniej liczby d, aby uniknac deadlocków, gdyż wartości a, b, c przylegających wpisów się powtarzały, co niestety blokowało cały program.
```javascript
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
```
[Pełny kod](/create_rels.js)

Jak widać każde zapytanie cypher tworzy relacje (jeśli nie istnieją) dla każdego pełnego podgrafu. Mniejsza ilość (większych) zapytań Cypher do Neo4j sprawiła że program zakończył się troche szybciej tworząc 142488 relacji
![Stats](/images/wykresy.png)

## Analiza grafu

Nastepnie można już przeglądać dane! Niestety rysowanie grafu zapytaniami Cypher JESZCZE nie jest zaimplemetnowane, więc rysować można wybierając elementy po indeksach. Na początek wybierzmy sobie losowy element:
![Stats](/images/1.png)

Następnie klikając na węzły będące z nim w relacji można dalej rozwijać graf:
![Stats](/images/2.png)

Gdy tworzą się nam agregacje elementów można je rozwinąc klikając na nie i wybierając "Select all"
![Stats](/images/3.png)

## Uruchamianie
Aby uruchomić program szukający w C należy oczywiście posiadać gcc oraz [sterowniki mongodb dla C](https://github.com/mongodb/mongo-c-driver/blob/master/README.md).
Należy również zmodyfikować search_fulls.c ustawiając zakres, liczbę wątków w zależności od ilości posiadanych rdzeni oraz szczegóły połączenia z bazą danych mongo

Kompilacja i uruchamianie:
```
gcc -Isrc --std=gnu99 /sciezka/do/mongo-c-driver/src/*.c -I /sciezka/do/mongo-c-driver/src/ search_fulls.c -lm -o search
./search
```

Po uruchomieniu należy zostawić komputer na pewien czas, w zależności od tego ile wyników ma on znaleźć.

Gdy mamy potrzebne wyniki, należy zainstalować [nodejs](http://nodejs.org/), [npm](https://npmjs.org/), a następnie w folderze projektu pobrać sterowniki do [mongodb](https://github.com/mongodb/node-mongodb-native) i [Neo4j](https://github.com/thingdom/node-neo4j):
```
npm install mongodb
npm install neo4j
```
Następnie zedytować szczegóły połączeń z bazami danych(najlepiej bazy lokalne) i kolejno stworzyć węzły i relacje(troszkę to potrwa zależnie od ilości danych):
```
node create_nodes.js
```
```
node create_rels.js
```
