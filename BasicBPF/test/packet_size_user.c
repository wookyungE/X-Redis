#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <sys/resource.h>
#include "bpf_insn.h"
//#define MAP_NAME "packet_size_map"
#define BUFFER_SIZE 1024
#define QUEUE_MAX_ENTRIES 10000
#define MAX_PORTS 10

#define NUM_PORTS 65535
#define INFO_NAME "port_info"


struct packet_info {
    __u64 offset;
    char size[10];
};

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

void bump_memlock_rlimit(void) {
    struct rlimit rlim_new = {
        .rlim_cur    = RLIM_INFINITY,
        .rlim_max    = RLIM_INFINITY,
    };

    if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
        fprintf(stderr, "WARNING: Failed to increase RLIMIT_MEMLOCK\n");
    }
}

int main(int argc, char **argv) {
    bump_memlock_rlimit();

//    int sock_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
        perror("socket failed");
        return 1;
    }

    const char *file = "packet_size_kern.o";


    struct bpf_object *obj = NULL;
    int prog_fd = load_ebpf_program(&obj,file);

#if 1
    if (prog_fd < 0) {
        fprintf(stderr, "failed to load eBPF program\n");
        close(sock_fd);
        return 1;
    }
    char MAP_NAME[MAX_PORTS][30] = {"packet_map_template0","packet_map_template1","packet_map_template2","packet_map_template3", "packet_map_template4","packet_map_template5","packet_map_template6","packet_map_template7", "packet_map_template8","packet_map_template9"};
    int map_fd[MAX_PORTS];

    for(int i = 0 ; i < MAX_PORTS; i++){
        printf("%s\n",MAP_NAME[i]);
        struct bpf_map *map = bpf_object__find_map_by_name(obj, MAP_NAME[i]);
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

    int port_info_fd = bpf_map__fd(port_info_map);
    if (port_info_fd < 0) {
        perror("Could not get 'port_info' map file descriptor");
        exit(EXIT_FAILURE);
    }

    __u32 key = htonl(0);
    __u64 value;
    
#if 1
#if 1
    if (setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_BPF, &prog_fd, sizeof(prog_fd)) < 0) {
        perror("setsockopt");
        close(sock_fd);
        return 1;
    }
#endif
    /* ... */

    int server_port = 3000;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sock_fd);
        return 1;
    }

    if (listen(sock_fd, 10) < 0) {
        perror("listen failed");
        close(sock_fd);
        return 1;
    }
    int reuse = 1;
     if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(sock_fd);
        return 1;
    }

    printf("Server is listening on port %d...\n", server_port);
    char buffer[BUFFER_SIZE];
    __u64 dst_port;

    int port_info[NUM_PORTS] = {0,};
    int dst_idx;
    struct packet_info pinfo;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        
        sleep(3);
        if (client_fd < 0) {
            perror("accept failed");
        } else {
            char client_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
            printf("Accepted connection from %s:%d\n", client_ip_str, ntohs(client_addr.sin_port));
            dst_port = ntohs(client_addr.sin_port);
            
            if(!port_info[dst_port]){
                if (bpf_map_lookup_elem(port_info_fd, &dst_port, &dst_idx) != 0) {
                    perror("Could not read 'port_info' BPF map");
                } 
                else {
                    printf("new_value for port %lld: %d\n", dst_port, dst_idx);
                }
            }
            else{
                dst_idx = port_info[dst_port];
            }

#if 1
            if (bpf_map_lookup_and_delete_elem(map_fd[dst_idx], NULL, &pinfo) != 0) {
                perror("Could not read BPF map");
            } else {
                printf("Current packet size: %s bytes, value offset: %lld, idx : %d\n",pinfo.size, pinfo.offset, dst_idx);
            }
#endif
            int recv_len = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
            if (recv_len == -1) {
                perror("recv");
            } else {
                buffer[recv_len] = '\0';
//                printf("Received message: %s\n", buffer);
            }
            close(client_fd);

        }
    }
#endif

    close(sock_fd);
#endif
    return 0;
}
