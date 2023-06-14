//
// Created by sergei on 19.06.22.
//

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "aioutils/uexcept.h"
#include "aioutils/unet.h"


using namespace aioutils;

namespace aioutils::unet {

    int setTcpKeepAlive(int sockfd)
    {
        int optval = 1;

        return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

    int setTcpKeepAliveCfg(int sockfd, const struct TcpKeepAliveConfig& cfg)
    {
        int rc;

        //first turn on keepalive
        rc = setTcpKeepAlive(sockfd);
        if (rc != 0) {
            return rc;
        }

        //set the keepalive options
        rc = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &cfg.keepcnt, sizeof cfg.keepcnt);
        if (rc != 0) {
            return rc;
        }

        rc = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &cfg.keepidle, sizeof cfg.keepidle);
        if (rc != 0) {
            return rc;
        }

        rc = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &cfg.keepintvl, sizeof cfg.keepintvl);
        if (rc != 0) {
            return rc;
        }

        return 0;
    }

    void setSocketReuseOptions(int socket) {
        int value = 1;

        if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
            uexcept::logErrno("While setting enabling SO_REUSEADDR");
        }

        if(setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) < 0) {
            uexcept::logErrno("While setting enabling SO_REUSEPORT");
        }
    }

    std::string inAddrToString(struct sockaddr_in sa)
    {
        char ipAddr[INET_ADDRSTRLEN];

        return std::string{inet_ntop(AF_INET, &sa.sin_addr, ipAddr, sizeof(ipAddr))};
    }

    int shutdownSocket(int fd, int how) {
        return shutdown(fd, how);
    }
}