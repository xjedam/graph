#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mongo.h"

#define FORKS 8
#define SIZE1 2000000
#define SIZE2 3000000
#define SIZE3 3000000
#define SIZE4 3000000
#define SIZE5 4000000
#define COLLPATH "squares.squares"
#define DBIP "192.168.1.121"
#define DBPORT 27017

mongo conn[1];
const char vars[7][3] = {"a", "b", "c", "d", "e", "f", "g"};

int is_square(long k){
    long m;
    m=(long)sqrt(k);
    if (m*m==k) {
        return 1;
    }
    return 0;
}

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

int main()
{
    long i=0,s, t;
    long a,b,c,d, e;
    pid_t childpid;
    long tab[10];

    mongo_init( conn );
    mongo_set_op_timeout( conn, 1000 );
    int status = mongo_client( conn, DBIP, DBPORT );

    if( status != MONGO_OK ) {
        switch ( conn->err ) {
            case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
            case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
            case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
            default: printf( "other error\n" ); return 1;
        }
    }
    printf("connected\n");

    do{
        childpid = fork();
        i++;
    }while(i<FORKS && childpid != 0);

    if(childpid < 0) {
        printf("forkerr\n");
        exit(0);
    }

    if(childpid == 0) {
        //printf("%d\n", i);
        for (a = i; a < SIZE1; a+=FORKS)
        {
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
    }

    mongo_destroy( conn );
    return 0;
}