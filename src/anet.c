/* anet.c -- Basic TCP socket stuff made a bit less boring
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fmacros.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "anet.h"

#ifdef USE_BPF
#include <linux/filter.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <stddef.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <bpf/libbpf.h>
#include <linux/bpf.h>
#include <bpf/bpf.h>
//#define MAP_NAME "packet_size_map"

// SMSHIN
pthread_mutex_t bpf_lock = PTHREAD_MUTEX_INITIALIZER;
__u64 temp[MAX_PORTS];
__u64 temp_idx;
#include "server.h"

#define MAX_PACKET_SIZE 64
#endif

static void anetSetError(char *err, const char *fmt, ...)
{
    va_list ap;

    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

int anetSetBlock(char *err, int fd, int non_block) {
    int flags;

    /* Set the socket blocking (if non_block is zero) or non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }

    if (non_block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetNonBlock(char *err, int fd) {
    return anetSetBlock(err,fd,1);
}

int anetBlock(char *err, int fd) {
    return anetSetBlock(err,fd,0);
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int anetKeepAlive(char *err, int fd, int interval)
{
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
    {
        anetSetError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return ANET_ERR;
    }

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Consider the socket in error state after three we send three ACK
     * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return ANET_ERR;
    }
#else
    ((void) interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return ANET_OK;
}

static int anetSetTcpNoDelay(char *err, int fd, int val)
{
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
    {
        anetSetError(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetEnableTcpNoDelay(char *err, int fd)
{
    return anetSetTcpNoDelay(err, fd, 1);
}

int anetDisableTcpNoDelay(char *err, int fd)
{
    return anetSetTcpNoDelay(err, fd, 0);
}


int anetSetSendBuffer(char *err, int fd, int buffsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
    {
        anetSetError(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetTcpKeepAlive(char *err, int fd)
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

/* Set the socket send timeout (SO_SNDTIMEO socket option) to the specified
 * number of milliseconds, or disable it if the 'ms' argument is zero. */
int anetSendTimeout(char *err, int fd, long long ms) {
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        anetSetError(err, "setsockopt SO_SNDTIMEO: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

/* anetGenericResolve() is called by anetResolve() and anetResolveIP() to
 * do the actual work. It resolves the hostname "host" and set the string
 * representation of the IP address into the buffer pointed by "ipbuf".
 *
 * If flags is set to ANET_IP_ONLY the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function. */
int anetGenericResolve(char *err, char *host, char *ipbuf, size_t ipbuf_len,
                       int flags)
{
    struct addrinfo hints, *info;
    int rv;

    memset(&hints,0,sizeof(hints));
    if (flags & ANET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
        anetSetError(err, "%s", gai_strerror(rv));
        return ANET_ERR;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
    }

    freeaddrinfo(info);
    return ANET_OK;
}

int anetResolve(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return anetGenericResolve(err,host,ipbuf,ipbuf_len,ANET_NONE);
}

int anetResolveIP(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return anetGenericResolve(err,host,ipbuf,ipbuf_len,ANET_IP_ONLY);
}

static int anetSetReuseAddr(char *err, int fd) {
    int yes = 1;
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

static int anetCreateSocket(char *err, int domain) {
    int s;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
        anetSetError(err, "creating socket: %s", strerror(errno));
        return ANET_ERR;
    }

    /* Make sure connection-intensive things like the redis benchmark
     * will be able to close/open sockets a zillion of times */
    if (anetSetReuseAddr(err,s) == ANET_ERR) {
        close(s);
        return ANET_ERR;
    }
    return s;
}

#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1
#define ANET_CONNECT_BE_BINDING 2 /* Best effort binding. */
static int anetTcpGenericConnect(char *err, char *addr, int port,
                                 char *source_addr, int flags)
{
    int s = ANET_ERR, rv;
    char portstr[6];  /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *bservinfo, *p, *b;

    snprintf(portstr,sizeof(portstr),"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr,portstr,&hints,&servinfo)) != 0) {
        anetSetError(err, "%s", gai_strerror(rv));
        return ANET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Try to create the socket and to connect it.
         * If we fail in the socket() call, or on connect(), we retry with
         * the next entry in servinfo. */
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;
        if (anetSetReuseAddr(err,s) == ANET_ERR) goto error;
        if (flags & ANET_CONNECT_NONBLOCK && anetNonBlock(err,s) != ANET_OK)
            goto error;
        if (source_addr) {
            int bound = 0;
            /* Using getaddrinfo saves us from self-determining IPv4 vs IPv6 */
            if ((rv = getaddrinfo(source_addr, NULL, &hints, &bservinfo)) != 0)
            {
                anetSetError(err, "%s", gai_strerror(rv));
                goto error;
            }
            for (b = bservinfo; b != NULL; b = b->ai_next) {
                if (bind(s,b->ai_addr,b->ai_addrlen) != -1) {
                    bound = 1;
                    break;
                }
            }
            freeaddrinfo(bservinfo);
            if (!bound) {
                anetSetError(err, "bind: %s", strerror(errno));
                goto error;
            }
        }
        if (connect(s,p->ai_addr,p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK)
                goto end;
            close(s);
            s = ANET_ERR;
            continue;
        }

        /* If we ended an iteration of the for loop without errors, we
         * have a connected socket. Let's return to the caller. */
        goto end;
    }
    if (p == NULL)
        anetSetError(err, "creating socket: %s", strerror(errno));

error:
    if (s != ANET_ERR) {
        close(s);
        s = ANET_ERR;
    }

end:
    freeaddrinfo(servinfo);

    /* Handle best effort binding: if a binding address was used, but it is
     * not possible to create a socket, try again without a binding address. */
    if (s == ANET_ERR && source_addr && (flags & ANET_CONNECT_BE_BINDING)) {
        return anetTcpGenericConnect(err,addr,port,NULL,flags);
    } else {
        return s;
    }
}

int anetTcpConnect(char *err, char *addr, int port)
{
    return anetTcpGenericConnect(err,addr,port,NULL,ANET_CONNECT_NONE);
}

int anetTcpNonBlockConnect(char *err, char *addr, int port)
{
    return anetTcpGenericConnect(err,addr,port,NULL,ANET_CONNECT_NONBLOCK);
}

int anetTcpNonBlockBindConnect(char *err, char *addr, int port,
                               char *source_addr)
{
    return anetTcpGenericConnect(err,addr,port,source_addr,
            ANET_CONNECT_NONBLOCK);
}

int anetTcpNonBlockBestEffortBindConnect(char *err, char *addr, int port,
                                         char *source_addr)
{
    return anetTcpGenericConnect(err,addr,port,source_addr,
            ANET_CONNECT_NONBLOCK|ANET_CONNECT_BE_BINDING);
}

int anetUnixGenericConnect(char *err, char *path, int flags)
{
    int s;
    struct sockaddr_un sa;

    if ((s = anetCreateSocket(err,AF_LOCAL)) == ANET_ERR)
        return ANET_ERR;

    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
    if (flags & ANET_CONNECT_NONBLOCK) {
        if (anetNonBlock(err,s) != ANET_OK) {
            close(s);
            return ANET_ERR;
        }
    }
    if (connect(s,(struct sockaddr*)&sa,sizeof(sa)) == -1) {
        if (errno == EINPROGRESS &&
            flags & ANET_CONNECT_NONBLOCK)
            return s;

        anetSetError(err, "connect: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return s;
}

int anetUnixConnect(char *err, char *path)
{
    return anetUnixGenericConnect(err,path,ANET_CONNECT_NONE);
}

int anetUnixNonBlockConnect(char *err, char *path)
{
    return anetUnixGenericConnect(err,path,ANET_CONNECT_NONBLOCK);
}

/* Like read(2) but make sure 'count' is read before to return
 * (unless error or EOF condition is encountered) */
int anetRead(int fd, char *buf, int count)
{
    ssize_t nread, totlen = 0;
    while(totlen != count) {
        nread = read(fd,buf,count-totlen);
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/* Like write(2) but make sure 'count' is written before to return
 * (unless error is encountered) */
int anetWrite(int fd, char *buf, int count)
{
    ssize_t nwritten, totlen = 0;
    while(totlen != count) {
        nwritten = write(fd,buf,count-totlen);
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

static int anetListen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog) {
    if (bind(s,sa,len) == -1) {
        anetSetError(err, "bind: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }

    if (listen(s, backlog) == -1) {
        anetSetError(err, "listen: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return ANET_OK;
}

static int anetV6Only(char *err, int s) {
    int yes = 1;
    if (setsockopt(s,IPPROTO_IPV6,IPV6_V6ONLY,&yes,sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return ANET_OK;
}

#ifdef USE_BPF
int load_ebpf_program(struct bpf_object **obj,const char *file) {
    int first_prog_fd = -1;

    *obj = bpf_object__open_file(file, NULL);
    if (libbpf_get_error(*obj) || !(*obj)) {
        fprintf(stderr, "error loading eBPF object file: %s\n", strerror(errno));
        return -1;
    }
    struct bpf_program *prog, *socket_filter_prog = NULL;
    bpf_object__for_each_program(prog, *obj) {
        const char *prog_name = bpf_program__name(prog);
        if (strcmp(prog_name, "drop_all") == 0) {
            socket_filter_prog = prog;
            bpf_program__set_type(socket_filter_prog, BPF_PROG_TYPE_SOCKET_FILTER);
            break;
        }
    }

    if (bpf_object__load(*obj) != 0) {
        fprintf(stderr, "error loading eBPF object file: %s\n", strerror(errno));
        bpf_object__close(*obj);
        return -1;
    }

    first_prog_fd = bpf_program__fd(socket_filter_prog);
    if (first_prog_fd < 0) {
        fprintf(stderr, "error with bpf_program__fd: %s\n", strerror(errno));
        bpf_object__close(*obj);
        return -1;
    }   

    return first_prog_fd;
}

void set_blocking_mode(int socket_fd) {
#if 1
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return;
    }

    flags &= ~O_NONBLOCK; // Clear the O_NONBLOCK flag

    if (fcntl(socket_fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return;
    }
#endif
}

void set_nonblocking_mode(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return;
    }

    flags |= O_NONBLOCK; // Set the O_NONBLOCK flag

    if (fcntl(socket_fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return;
    }
}


#endif

static int _anetTcpServer(char *err, int port, char *bindaddr, int af, int backlog)
{
    int s = -1, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    snprintf(_port,6,"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    /* No effect if bindaddr != NULL */

    if ((rv = getaddrinfo(bindaddr,_port,&hints,&servinfo)) != 0) {
        anetSetError(err, "%s", gai_strerror(rv));
        return ANET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;
#ifdef USE_BPF
        file = "/home/ljkim/bpf/bpf_redis/bpf_plus_redis/packet_size_kern.o";
        obj = NULL;
        
        prog_fd = load_ebpf_program(&obj,file);

        if (prog_fd < 0) {
            fprintf(stderr, "failed to load eBPF program\n");
            close(s);
            return 1;
        }
        
        //char MAP_NAME[MAX_PORTS][30] = {"packet_map_template0","packet_map_template1","packet_map_template2","packet_map_template3", "packet_map_template4","packet_map_template5","packet_map_template6","packet_map_template7", "packet_map_template8","packet_map_template9"};
        //char MAP_NAME[MAX_PORTS][30] = {"packet_map_template0","packet_map_template1","packet_map_template2","packet_map_template3", "packet_map_template4","packet_map_template5","packet_map_template6","p    acket_map_template7", "packet_map_template8","packet_map_template9", "packet_map_template10","packet_map_template11","packet_map_template12","packet_map_template13", "packet_map_template14","packet_map_template15","packet_map_template16","packet_map_template17", "packet_map_template18","packet_map_template19", "packet_map_template20","packet_map_template21","packet_map_template22","packet_map_template23", "packet_map_template24","packet_map_template25","packet_map_template26","packet_map_template27", "packet_map_template28","packet_map_template29", "packet_map_template30","packet_map_template31","packet_map_template32","packet_map_template33", "packet_map_template34","packet_map_template35","packet_map_template36","packet_map_template37", "packet_map_template38","packet_map_template39", "packet_map_template40","packet_map_template41","packet_map_template42","packet_map_template43", "packet_map_template44","packet_map_template45","packet_map_template46","packet_map_template47", "packet_map_template48","packet_map_template49"};
		char MAP_NAME[MAX_PORTS][30] = {"packet_map_template0","packet_map_template1","packet_map_template2","packet_map_template3", "packet_map_template4","packet_map_template5","packet_map_template6","packet_map_template7", "packet_map_template8","packet_map_template9", "packet_map_template10","packet_map_template11","packet_map_template12","packet_map_template13", "packet_map_template14","packet_map_template15","packet_map_template16","packet_map_template17", "packet_map_template18","packet_map_template19"};        

        for(int i = 0 ; i < MAX_PORTS; i++){
            map = bpf_object__find_map_by_name(obj, MAP_NAME[i]);
            if (!map) {
                printf("Could not find BPF map\n");
                return 1;
            }

            map_fd[i] = bpf_map__fd(map);
            if (map_fd[i] < 0) {
                perror("Could not get map file descriptor");
                exit(EXIT_FAILURE);
            }
        }

        struct bpf_map *port_info_map = bpf_object__find_map_by_name(obj, INFO_NAME);
        if (!port_info_map) {
            printf("Could not find BPF map 'port_info'\n");
            return 1;
        }

        port_info_fd = bpf_map__fd(port_info_map);
        if (port_info_fd < 0) {
            perror("Could not get 'port_info' map file descriptor");
            exit(EXIT_FAILURE);
        }



        if (setsockopt(s, SOL_SOCKET, SO_ATTACH_BPF, &prog_fd, sizeof(prog_fd)) < 0) {
            perror("setsockopt");
            exit(1);
        }
//        set_blocking_mode(s);
//        set_nonblocking_mode(s);
#endif
        if (af == AF_INET6 && anetV6Only(err,s) == ANET_ERR) goto error;
        if (anetSetReuseAddr(err,s) == ANET_ERR) goto error;
        if (anetListen(err,s,p->ai_addr,p->ai_addrlen,backlog) == ANET_ERR) goto error;
        goto end;
    }
    if (p == NULL) {
        anetSetError(err, "unable to bind socket, errno: %d", errno);
        goto error;
    }

error:
    if (s != -1) close(s);
    s = ANET_ERR;
end:
    freeaddrinfo(servinfo);
    return s;
}

int anetTcpServer(char *err, int port, char *bindaddr, int backlog)
{
    return _anetTcpServer(err, port, bindaddr, AF_INET, backlog);
}

int anetTcp6Server(char *err, int port, char *bindaddr, int backlog)
{
    return _anetTcpServer(err, port, bindaddr, AF_INET6, backlog);
}

int anetUnixServer(char *err, char *path, mode_t perm, int backlog)
{
    int s;
    struct sockaddr_un sa;

    if ((s = anetCreateSocket(err,AF_LOCAL)) == ANET_ERR)
        return ANET_ERR;

    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
    if (anetListen(err,s,(struct sockaddr*)&sa,sizeof(sa),backlog) == ANET_ERR)
        return ANET_ERR;
    if (perm)
        chmod(sa.sun_path, perm);
    return s;
}

// SMSHIN
long long sm_ustime(void) {
    struct timeval tv;
    long long ust; 

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust; 
}

static int anetGenericAccept(char *err, int s, struct sockaddr *sa, socklen_t *len, struct list *clients) {
    int fd;
	//long long stime = sm_ustime();
    while(1) {
        fd = accept(s,sa,len);

        if (fd == -1) {
            if (errno == EINTR)
                continue;
            else {
                anetSetError(err, "accept: %s", strerror(errno));
                return ANET_ERR;
            }
        }
        break;
    }

	//long long stime = sm_ustime();

#ifdef USE_BPF
    //usleep(100000);
    //usleep(10000);
    struct sockaddr_in *sa_in = (struct sockaddr_in *)sa;
    __u64 dst_port = ntohs(sa_in->sin_port);
    int dst_idx = -1;
    //printf("fd : %d, dst_port : %d\n",port_info_fd,dst_port);
	
// SMSHIN
    //pthread_mutex_lock(&bpf_lock);      
    if(bpf_map_lookup_elem(port_info_fd, &dst_port, &dst_idx))
    {
		listIter *iter = listGetIterator(clients, 0);
    	listNode *node;
		client *cli;
		struct sockaddr_in addr;
		socklen_t addr_size;
		char clientip[20];
		int clientport;
		__u64 key = 0, next_key = 0, tmp = 0;
    	void *value;
		int clients_port_idx = 0, new_value = 0;

    	while ((node = listNext(iter)) != NULL) {
        	cli = listNodeValue(node);
			addr_size = sizeof(struct sockaddr_in);
			getpeername(cli->fd, (struct sockaddr *)&addr, &addr_size);

			inet_ntop(AF_INET, &addr.sin_addr, clientip, sizeof(clientip));
			clientport = ntohs(addr.sin_port);
			clients_port[clients_port_idx++] = clientport;
    	}

		while (bpf_map_get_next_key(port_info_fd, &key, &next_key) == 0) {
        	if (bpf_map_lookup_elem(port_info_fd, &next_key, &value)) {
            	perror("bpf_map_lookup_elem");
        	}

			int check = 0;
			key = next_key;

			for (int i = 0; i < clients_port_idx; i++) {
            	tmp = (__u64) clients_port[i];
		
				if (key == tmp) {
					check = 1;
					break;
				}
        	}
	
			if(!check) {
				empty_hash[(int)value] = -1;
				port_info[fd] = -1;
				bpf_map_delete_elem(port_info_fd, &key);
			}
    	}

		for (int i = 0; i < MAX_PORTS; i++) {
			if (empty_hash[i] <= 0) {
				new_value = i;
				break;
			}
		}

        if(bpf_map_update_elem(port_info_fd, &dst_port, &new_value, BPF_ANY) != 0) {
			perror("bpf_map_update_elem");
		}
	}

	if(port_info[fd] <= 0){
        if (bpf_map_lookup_elem(port_info_fd, &dst_port, &dst_idx) != 0) {
            perror("Could not read 'port_info' BPF map");
        }
        else {
            //printf("new_value for port %lld: %d, client fd : %d\n", dst_port, dst_idx, fd);
		}
    }
    else{
        dst_idx = port_info[fd];
    }
    port_info[fd] = dst_idx;
	empty_hash[dst_idx] = dst_port;

	//printf("map_fd[port_info[fd]] : %d\n", map_fd[port_info[fd]]);
    //pthread_mutex_unlock(&bpf_lock);

// SMSHIN
/*
	printf("start\n");
	for (int i = 0; i < MAX_PORTS; i++) {
		printf("empty hash[%d] : %d\n", i, empty_hash[i]);
	}  	



	__u64 key = 0;
	__u64 next_key = 0;
	void *value;

	while (bpf_map_get_next_key(port_info_fd, &key, &next_key) == 0) {
		if (bpf_map_lookup_elem(port_info_fd, &next_key, &value) != 0) {
			perror("bpf_map_lookup_elem");
			return 1;
		}

		printf("Key: %u, Value: %d\n", next_key, value);

		key = next_key;
	}
*/
#endif

	//long long duration = sm_ustime() - stime;
	//printf("%lld\n", duration);

    return fd;
}

int anetTcpAccept(char *err, int s, char *ip, size_t ip_len, int *port, struct list *clients) {
    int fd;
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);
	// SMSHIN
	//if ((fd = anetGenericAccept(err,s,(struct sockaddr*)&sa,&salen)) == -1)
    if ((fd = anetGenericAccept(err,s,(struct sockaddr*)&sa,&salen, clients)) == -1)
        return ANET_ERR;

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
    }

    return fd;
}

int anetUnixAccept(char *err, int s) {
    int fd;
    struct sockaddr_un sa;
    socklen_t salen = sizeof(sa);
	// SMSHIN
    //if ((fd = anetGenericAccept(err,s,(struct sockaddr*)&sa,&salen)) == -1)
	if ((fd = anetGenericAccept(err,s,(struct sockaddr*)&sa,&salen, 0)) == -1)
        return ANET_ERR;

    return fd;
}

int anetPeerToString(int fd, char *ip, size_t ip_len, int *port) {
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd,(struct sockaddr*)&sa,&salen) == -1) goto error;
    if (ip_len == 0) goto error;

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin_port);
    } else if (sa.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
    } else if (sa.ss_family == AF_UNIX) {
        if (ip) strncpy(ip,"/unixsocket",ip_len);
        if (port) *port = 0;
    } else {
        goto error;
    }
    return 0;

error:
    if (ip) {
        if (ip_len >= 2) {
            ip[0] = '?';
            ip[1] = '\0';
        } else if (ip_len == 1) {
            ip[0] = '\0';
        }
    }
    if (port) *port = 0;
    return -1;
}

/* Format an IP,port pair into something easy to parse. If IP is IPv6
 * (matches for ":"), the ip is surrounded by []. IP and port are just
 * separated by colons. This the standard to display addresses within Redis. */
int anetFormatAddr(char *buf, size_t buf_len, char *ip, int port) {
    return snprintf(buf,buf_len, strchr(ip,':') ?
           "[%s]:%d" : "%s:%d", ip, port);
}

/* Like anetFormatAddr() but extract ip and port from the socket's peer. */
int anetFormatPeer(int fd, char *buf, size_t buf_len) {
    char ip[INET6_ADDRSTRLEN];
    int port;

    anetPeerToString(fd,ip,sizeof(ip),&port);
    return anetFormatAddr(buf, buf_len, ip, port);
}

int anetSockName(int fd, char *ip, size_t ip_len, int *port) {
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getsockname(fd,(struct sockaddr*)&sa,&salen) == -1) {
        if (port) *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return -1;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
    }
    return 0;
}

int anetFormatSock(int fd, char *fmt, size_t fmt_len) {
    char ip[INET6_ADDRSTRLEN];
    int port;

    anetSockName(fd,ip,sizeof(ip),&port);
    return anetFormatAddr(fmt, fmt_len, ip, port);
}
