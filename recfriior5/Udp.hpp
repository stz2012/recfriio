/* pubic domain */
#ifndef _UDP_H
#define _UDP_H

#include <netdb.h>
#include <arpa/inet.h>
#include <string>


class Udp
{
    std::string host_to;
    int port_to;

    int soc ;
    struct sockaddr_in addr ;

    /** ログ出力先 */
    std::ostream *log;

public:

    Udp();
    virtual ~Udp();

    virtual void setLog(std::ostream *plog) {
        log = plog;
    };

    void init( const std::string& host, int port );
    ssize_t send( const uint8_t* buf, const size_t len );
    void shutdown();
};

#endif
