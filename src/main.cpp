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
#include <vector>
#include <algorithm>
#define BUFSIZE 1024
#define NLISTEN 1
#define T1 1000
char *program_name;
#include "skel.h"

struct client_set
{
    struct sockaddr_in peer;
    SOCKET s;
    SOCKET c;
    socklen_t peerlen;
};
struct timeval tv;
fd_set allfd;
fd_set readfd;
char *qhname;
char *qsname;

int main( int argc, char **argv )
{
    std::vector<client_set> vcs;
    client_set cs;
    char buf[BUFSIZE];
    char *hname;
    char *sname;
    int rc;
    SOCKET s;
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
    tv.tv_sec = T1;
    tv.tv_usec = 0;
    FD_ZERO( &allfd );
    FD_SET( s, &allfd );
    do
    {
        readfd = allfd;
        if ( vcs.empty() )
        {
            rc = select( s + 1, &readfd, NULL, NULL, &tv );
        }
        else
            rc = select( vcs.back().c + 1, &readfd, NULL, NULL, &tv );
     
        #ifdef DEBUG
        printf( "select return - %d\n", rc );
        #endif
     
        if ( rc < 0 )
            error( 1, errno, (char*)"select call error\n" );
        if ( FD_ISSET( s, &readfd ) )
        {
            cs.peerlen = sizeof( cs.peer );
            cs.s = accept( s, (struct sockaddr * )&cs.peer, &cs.peerlen );
            if ( !isvalidsock( cs.s ) )
                error( 1, errno, (char*)"accept call error\n" );
            cs.c = tcp_client( qhname, qsname );
            vcs.push_back( cs );
            FD_SET( vcs.back().s, &allfd );
            FD_SET( vcs.back().c, &allfd );
        }
        for ( auto ics : vcs )
        {   
            if ( FD_ISSET( ics.c, &readfd ) )
            {
                rc = recv( ics.c, &buf, sizeof(buf), 0 );
                if ( rc < 0 ) error( 1, errno, (char*)"recv call error\n" );
                if ( rc == 0 )
                {
                    #ifdef DEBUG
                    error( 0, errno, (char*)"c recv is 0\n" );
                    #endif
                    CLOSE( ics.s );
                    CLOSE( ics.c );
                    FD_CLR( ics.s, &allfd );
                    FD_CLR( ics.c, &allfd );
                    vcs.erase( std::remove_if( vcs.begin(), vcs.end(), [&ics](const client_set &ecs){return ics.s == ecs.s; } ), vcs.end() );
                    break;
                }
                if ( rc > 0 ) rc = send( ics.s, buf, rc, 0 );
                if ( rc < 0 ) error( 1, errno, (char*)"send call error\n" );
                if ( rc == 0 ) error( 0, errno, (char*)"c send is 0\n" );
            }
            if ( FD_ISSET( ics.s, &readfd ) )
            {
                rc = recv( ics.s, &buf, sizeof(buf), 0 );
                if ( rc < 0 ) error( 1, errno, (char*)"recv call error\n" );
                if ( rc == 0 ) 
                {
                    #ifdef DEBUG
                    error( 0, errno, (char*)"s recv is 0\n" );
                    #endif
                    CLOSE( ics.s );
                    CLOSE( ics.c );
                    FD_CLR( ics.s, &allfd );
                    FD_CLR( ics.c, &allfd );
                    vcs.erase( std::remove_if( vcs.begin(), vcs.end(), [&ics](const client_set &ecs){return ics.s == ecs.s; } ), vcs.end() );
                    break;
                }
                if ( rc > 0 ) rc = send( ics.c, buf, rc, 0 );
                if ( rc < 0 ) error( 1, errno, (char*)"send call error\n" );
                if ( rc == 0 ) error( 0, errno, (char*)"s send is 0\n" );
                #ifdef DEBUG
                printf( "recv s - %d\t", rc );
                #endif
                printf("<request><client>%s:%d</client><message>", inet_ntoa( ics.peer.sin_addr ), ntohs( ics.peer.sin_port ) );
                for ( int i = 5; i < rc; i++ )
                    printf( "%c", buf[i] );
                printf("</message></request>\n");
            }
        }
        tv.tv_sec = T1;
        #ifdef DEBUG
        printf( "tv.sec=%ld\ttv.u=%ld\n", tv.tv_sec, tv.tv_usec );
        #endif
    } while (1);
    EXIT( 0 );
}