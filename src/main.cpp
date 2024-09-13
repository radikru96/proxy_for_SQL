#include <iostream>
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
#include <fstream>
#include <csignal>
#define BUFSIZE 1024
#define PATH "log.txt"
#define NLISTEN 1
#define T1 1000
char *program_name;
#include "skel.h"

enum class Transaction {TODB,FROMDB,NO};

struct client_set
{
    struct sockaddr_in peer;
    SOCKET s;
    SOCKET c;
    socklen_t peerlen;
};

std::vector<client_set> vcs;
std::ofstream fout;

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    for( auto i : vcs )
    {
        CLOSE( i.c );
        CLOSE( i.s );
    }
    fout.close();
    EXIT( 0 );
}

int main( int argc, char **argv )
{
    std::signal( SIGINT, signalHandler );
    client_set cs;
    struct timeval tv;
    char buf[BUFSIZE];
    fd_set allfd;
    fd_set readfd;
    char *hname;
    char *sname;
    char *qhname;
    char *qsname;
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
        std::cout << "Too few args\n";
        return 1;
    }
    fout.open( PATH, std::ofstream::app);
    if ( !fout.is_open() )
    {
        std::cout << "open file error\n";
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
        std::cout << "select return : " << rc << std::endl;
        #endif
     
        if ( rc < 0 )
            error( 1, errno, (char*)"select call error\n" );
        if ( FD_ISSET( s, &readfd ) ) // Check for new client
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
        for ( auto ics : vcs ) // Connections run
        {   
            bool is_break = false;
            Transaction tr;
            int transaction_count = 0;
            if ( FD_ISSET( ics.c, &readfd ) ) {
                tr=Transaction::FROMDB;
                transaction_count++;
            }
            if ( FD_ISSET( ics.s, &readfd ) ) {
                tr=Transaction::TODB;
                transaction_count++;
            }
            if ( tr != Transaction::NO ){
                while ( transaction_count > 0 ){
                    transaction_count--;
                    rc = recv( tr == Transaction::FROMDB ? ics.c : ics.s , &buf, sizeof(buf), 0 );
                    if ( rc <= 0 )
                    {
                        if ( rc < 0 ) error( 0, errno, (char*)"recv call error\n" ); // catching connection failure
                        #ifdef DEBUG
                        else error( 0, errno, (char*) (tr == Transaction::FROMDB ? "c recv is 0\n" : "s recv is 0\n") );
                        #endif
                        CLOSE( ics.s );
                        CLOSE( ics.c );
                        FD_CLR( ics.s, &allfd );
                        FD_CLR( ics.c, &allfd );
                        vcs.erase( std::remove_if( vcs.begin(), vcs.end(), [&ics](const client_set &ecs){return ics.s == ecs.s; } ), vcs.end() );
                        is_break = true;
                        break;
                    }
                    if ( rc > 0 ) rc = send( tr == Transaction::FROMDB ? ics.s : ics.c , buf, rc, 0 );
                    if ( rc < 0 ) error( 1, errno, (char*)"send call error\n" );
                    if ( rc == 0 ) error( 0, errno, (char*)(tr == Transaction::FROMDB ? "c send is 0\n" : "s send is 0\n") );
                    if ( tr == Transaction::TODB ){
                        #ifdef DEBUG
                        std::cout << "recv s - " << rc << std::endl;
                        #endif
                        if ( ((int)buf[5]) > 32 )
                        {
                            #ifdef DEBUG
                            std::cout << inet_ntoa( ics.peer.sin_addr ) << ':' << ntohs( ics.peer.sin_port ) << " \"";
                            #endif
                            fout << inet_ntoa( ics.peer.sin_addr ) << ':' << ntohs( ics.peer.sin_port ) << " \"";
                            for ( int i = 5; i < rc; i++ ){
                                fout << buf[i];
                                #ifdef DEBUG
                                std::cout << buf[i];
                                #endif
                            }
                            fout << "\"\n";
                            #ifdef DEBUG
                            std::cout << "\"" << std::endl;
                            #endif
                        }
                    }
                    else tr = Transaction::TODB;
                }
                if ( is_break )
                    break;
            }
        }
        tv.tv_sec = T1;
        #ifdef DEBUG
        printf( "tv.sec=%ld\ttv.u=%ld\n", tv.tv_sec, tv.tv_usec );
        #endif
    } while ( 1 );
}