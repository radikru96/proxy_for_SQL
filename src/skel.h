#ifndef __SKEL_H__
#define __SKEL_H__
#define INIT()  (program_name = strrchr( argv[ 0 ], '/' )) ? program_name++ : ( program_name = argv[ 0 ] )
#define EXIT(s) exit( s )
#define CLOSE(s) if ( close( s ) ) error( 1, errno, (char*)"error close " )
#define set_errno(e) errno = ( e )
#define isvalidsock(s) ( ( s ) >= 0 )
#define NLISTEN 1
// #include <socket.h>
typedef int SOCKET;
int readline( SOCKET s, char *buf, size_t len );
void error( int status, int err, char *format, ... );
int readn( SOCKET s, char *buf, size_t len);
int readvrec( SOCKET s, char *buf, size_t len );
static void set_address( char *host, char *port, struct sockaddr_in *sap, char *protocol );
SOCKET tcp_server( char *host, char *port );
SOCKET tcp_client( char *host, char *port );
SOCKET udp_server( char *host, char *port );
SOCKET udp_client( char *host, char *port, struct sockaddr_in *sap );

int readline( SOCKET fd, char *bufptr, size_t len )
{
    char *bufx = bufptr;
    static char *bp;
    static int cnt = 0;
    static char b[ 1500 ];
    char c;
    while ( --len > 0 )
    {
        if ( --cnt <= 0 )
        {
            cnt = recv( fd, b, sizeof( b ), 0 );
            if ( cnt < 0 )
            {
                if ( errno == EINTR )
                {
                    len++;
                    continue;
                }
                return -1;
            }
            if ( cnt == 0 )
                return 0;
            bp = b;
        }
        c = *bp++;
        *bufptr++ = c;
        if ( c == '\n' )
        {
            *bufptr = '\0';
            return bufptr - bufx;
        }
    }
    set_errno( EMSGSIZE );
    return -1;
}

void error( int status, int err, char *fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "%s: ", program_name );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
    if ( err )
        fprintf( stderr, ": %s (%d)\n", strerror( err ), err );
    if ( status )
        EXIT( status );
}

int readn( SOCKET fd, char *bp, size_t len)
{
    int cnt;
    int rc;
    cnt = len;
    while ( cnt > 0 )
    {
        rc = recv( fd, bp, cnt, 0 );
        if ( rc < 0 )
        {
            if ( errno == EINTR )
                continue;
            return -1;
        }
        if ( rc == 0 )
            return len - cnt;
        bp += rc;
        cnt -= rc;
    }
    return len;
}

int readvrec( SOCKET fd, char *bp, size_t len )
{
    u_int32_t reclen;
    int rc;
    rc = readn( fd, ( char * )&reclen, sizeof( u_int32_t ) );
    if ( rc != sizeof( u_int32_t ) )
        return rc < 0 ? -1 : 0;
    reclen = ntohl( reclen );
    if ( reclen > len )
    {
        while ( reclen > 0 )
        {
            rc = readn( fd, bp, len );
            if ( (std::size_t)rc != len )
                return rc < 0 ? -1 : 0;
            reclen -= len;
            if ( reclen < len )
                len = reclen;
        }
        set_errno( EMSGSIZE );
        return -1;
    }
    rc = readn( fd, bp, reclen );
    if ( (uint32_t)rc != reclen )
        return rc < 0 ? -1 : 0;
    return rc;
}

static void set_address( char *hname, char *sname, struct sockaddr_in *sap, char *protocol )
{
    struct servent *sp;
    struct hostent *hp;
    char *endptr;
    short port;
    bzero( sap, sizeof( *sap ) );
    sap->sin_family = AF_INET;
    if ( hname != NULL )
    {
        if ( !inet_aton( hname, &sap->sin_addr ) )
        {
            hp = gethostbyname( hname );
            if ( hp == NULL )
                error( 1, 0, (char *)"unknown host: %s\n", hname );
            sap->sin_addr = *( struct in_addr * )hp->h_addr;
        }
    }
    else
        sap->sin_addr.s_addr = htonl( INADDR_ANY );
    port = strtol( sname, &endptr, 0 );
    if ( *endptr == '\0' )
        sap->sin_port = htons( port);
    else
    {
        sp = getservbyname( sname, protocol );
        if ( sp == NULL )
            error( 1, 0, (char *)"unknown service: %s\n", sname );
        sap->sin_port = sp->s_port;
    }
}

SOCKET tcp_server( char *hname, char *sname )
{
    struct sockaddr_in local;
    SOCKET s;
    const int on = 1;
    set_address( hname, sname, &local, (char *)"tcp" );
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( !isvalidsock( s ) )
        error( 1, errno, (char *)"socket call error" );
    if ( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, ( char * )&on, sizeof( on ) ) )
        error( 1, errno, (char *)"setsockopt call error" );
    if ( bind( s, ( struct sockaddr * ) &local, sizeof( local ) ) )
        error( 1, errno, (char *)"bind call error" );
    if ( listen( s, NLISTEN ) )
        error( 1, errno, (char *)"listen call error" );
    return s;
}

SOCKET tcp_client( char *hname, char *sname )
{
    struct sockaddr_in peer;
    SOCKET s;
    set_address( hname, sname, &peer, (char *)"tcp" );
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( !isvalidsock( s ) )
        error( 1, errno, (char *)"socket call error");
    if ( connect( s, ( struct sockaddr * )&peer, sizeof( peer ) ) )
        error( 1, errno, (char *)"connect call error");
    return s;
}

SOCKET udp_server( char *hname, char *sname )
{
    SOCKET s;
    struct sockaddr_in local;
    set_address( hname, sname, &local, (char *)"udp" );
    s = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( !isvalidsock( s ) )
        error( 1, errno, (char *)"socket call error" );
    if ( bind( s, ( struct sockaddr * ) &local, sizeof( local ) ) )
        error( 1, errno, (char *)"bind call error");
    return s;
}

SOCKET udp_client( char *hname, char *sname, struct sockaddr_in *sap )
{
    SOCKET s;
    set_address( hname, sname, sap, (char *)"udp" );
    s = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( !isvalidsock( s ) )
        error( 1, errno, (char *)"socket call error");
    return s;
}

#endif /* __SKEL_H__ */