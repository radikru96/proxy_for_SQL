#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define NLISTEN 1
#define T1 1000
char *program_name;
#include "skel.h"

struct timeval tv;
fd_set allfd;
fd_set readfd;
char *qhname;
char *qsname;

static void server(SOCKET s, struct sockaddr_in *peerp )
{
    char buf[1024];
    int rc;
    SOCKET s2;
    s2 = tcp_client( qhname, qsname );
    FD_SET( s2, &allfd );
    while (1)
    {
        readfd = allfd;
        rc = select( s2 + 1, &readfd, NULL, NULL, &tv );
        #ifdef DEBUG
        printf( "select return - %d\n", rc );
        #endif
        if ( rc < 0 )
            error( 1, errno, (char*)"select call error\n" );
        if ( FD_ISSET( s2, &readfd ) )
        {
            rc = recv( s2, &buf, sizeof(buf), 0 );
            if ( rc < 0 ) error( 1, errno, (char*)"recv call error\n" );
            if ( rc == 0 ) error( 1, errno, (char*)"s2 recv is 0\n" );
            if ( rc > 0 ) rc = send( s, buf, rc, 0 );
            if ( rc < 0 ) error( 1, errno, (char*)"send call error\n" );
            if ( rc == 0 ) error( 1, errno, (char*)"s2 send is 0\n" );
            #ifdef DEBUG
            printf( "recv s2 - %d\t", rc );
            #endif
            for ( int i = 0; i < rc; i++ )
                printf( "%c", buf[i] );
            printf("\n");
        }
        if ( FD_ISSET( s, &readfd ) )
        {
            rc = recv( s, &buf, sizeof(buf), 0 );
            if ( rc < 0 ) error( 1, errno, (char*)"recv call error\n" );
            if ( rc == 0 ) error( 1, errno, (char*)"s recv is 0\n" );
            if ( rc > 0 ) rc = send( s2, buf, rc, 0 );
            if ( rc < 0 ) error( 1, errno, (char*)"send call error\n" );
            if ( rc == 0 ) error( 1, errno, (char*)"s send is 0\n" );
            #ifdef DEBUG
            printf( "recv s1 - %d\t", rc );
            #endif
            for ( int i = 0; i < rc; i++ )
                printf( "%c", buf[i] );
            printf("\n");
        }
        tv.tv_sec = T1;
        #ifdef DEBUG
        printf( "tv.sec=%ld\ttv.u=%ld\n", tv.tv_sec, tv.tv_usec );
        #endif
    }
    CLOSE( s2 );
}

int main( int argc, char **argv )
{
    struct sockaddr_in peer;
    char *hname;
    char *sname;
    socklen_t peerlen;
    SOCKET s;
    SOCKET s1;
    INIT();
    if ( argc == 4 )
    {
        hname = NULL;
        sname = argv[ 1 ];
        qhname = argv[ 2 ];
        qsname = argv[ 3 ];
    }
    else
    {
        printf("Too few args\n");
        return 1;
    }
    s = tcp_server( hname, sname);
    do
    {
        peerlen = sizeof( peer );
        s1 = accept( s, (struct sockaddr * )&peer, &peerlen );
        if ( !isvalidsock( s1 ) )
            error( 1, errno, (char*)"accept call error\n" );
        tv.tv_sec = T1;
        tv.tv_usec = 0;
        FD_ZERO( &allfd );
        FD_SET( s1, &allfd );
        server( s1, &peer );
        CLOSE( s1 );
    } while (1);
    EXIT( 0 );
}