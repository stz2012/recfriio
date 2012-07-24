/* pubic domain */
#include "Udp.hpp"

#include <iostream>
#include <string.h>

Udp::Udp()
    : soc( -1 ), log( NULL )
{}


Udp::~Udp()
{
    shutdown();
}

void Udp::init( const std::string& host, int port )
{
    if( soc != -1 ) return;

    host_to = host;
    port_to =  port;

    if (log) *log << "creating socket...";

    in_addr ia;
    ia.s_addr = inet_addr( host_to.c_str() );
    if( ia.s_addr == INADDR_NONE ){
        hostent *hoste = gethostbyname( host_to.c_str() );
        if( ! hoste ) throw( "failed to get host by name" );
        ia.s_addr = *( in_addr_t* )( hoste->h_addr_list[ 0 ] );
    }

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET ;
    addr.sin_port = htons( port_to );
    addr.sin_addr.s_addr = ia.s_addr;

    if( ( soc = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
        throw( "failed to create socket" );
    }

    if (log) *log << "done. address = " << inet_ntoa( ia ) << ":" << port_to << std::endl;
}

ssize_t Udp::send( const uint8_t* buf, const size_t len )
{
    if( soc == -1 ) return 0;

    return sendto( soc, buf, len, 0, ( struct sockaddr * )&addr, sizeof( addr ) );
}


void Udp::shutdown()
{
    if( soc != -1 ){
        if (log) *log << "closing socket...";
        close( soc );
        if (log) *log << "done.\n";
        soc = -1;
    }
}
