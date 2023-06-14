//
// Created by sergei on 19.06.22.
//

#ifndef AIOUTILS_UNET_H
#define AIOUTILS_UNET_H
namespace aioutils::unet {

    struct TcpKeepAliveConfig {
        /** The time (in seconds) the connection needs to remain
         * idle before TCP starts sending keepalive probes (TCP_KEEPIDLE socket option)
         */
        int keepidle;
        /** The maximum number of keepalive probes TCP should
         * send before dropping the connection. (TCP_KEEPCNT socket option)
         */
        int keepcnt;

        /** The time (in seconds) between individual keepalive probes.
         *  (TCP_KEEPINTVL socket option)
         */
        int keepintvl;
    };

    void setSocketReuseOptions(int socket);
    std::string inAddrToString(struct sockaddr_in sa);
    int shutdownSocket(int fd, int how = SHUT_RDWR);
    int setTcpKeepAliveCfg(int sockfd, const struct TcpKeepAliveConfig& cfg);
}
#endif //AIOUTILS_UNET_H
