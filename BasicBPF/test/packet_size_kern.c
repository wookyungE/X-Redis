//#include <linux/in.h>
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_endian.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <string.h>
#include <linux/in.h>
#include "bpf_legacy.h"
#define QUEUE_MAX_ENTRIES 10000
#define MAX_PORTS 10
struct packet_info {
    __u64 offset;
    char size[10]; // Change this to a character array
};

union port_t{
        __be32 ports;
        __be16 port16[2];
};

struct bpf_map{
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __type(value, struct packet_info);
    __uint(max_entries, QUEUE_MAX_ENTRIES);
};
struct bpf_map packet_map_template0 SEC(".maps"), packet_map_template1 SEC(".maps"), packet_map_template2 SEC(".maps"), packet_map_template3 SEC(".maps"), packet_map_template4 SEC(".maps"), packet_map_template5 SEC(".maps"), packet_map_template6 SEC(".maps"), packet_map_template7 SEC(".maps"), packet_map_template8 SEC(".maps"), packet_map_template9 SEC(".maps");


struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key,__u64);
    __type(value,int);
    __uint(max_entries,MAX_PORTS);
}port_info SEC(".maps");


int port_idx = 0;
//__u64 port_idx = 0;
static inline int bpf_memcmp(const void *s1, const void *s2, unsigned int n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    for (unsigned int i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }

    return 0;
}

static int find_index(__u64 new_port){
    int* value;
    value = bpf_map_lookup_elem(&port_info, &new_port);
    if(value){
        return *value;
    }
    else{
        int new_value = port_idx++;
        bpf_map_update_elem(&port_info, &new_port, &new_value, BPF_ANY);
        return new_value;
    }
    return -1;
}



static inline void bpf_push_elem(int hash_idx, struct packet_info* pinfo){
    if (hash_idx == 0) {
        bpf_map_push_elem(&packet_map_template0, pinfo, BPF_ANY);
    } else if (hash_idx == 1) {
        bpf_map_push_elem(&packet_map_template1, pinfo, BPF_ANY);
    } else if (hash_idx == 2) {
        bpf_map_push_elem(&packet_map_template2, pinfo, BPF_ANY);
    } else if (hash_idx == 3) {
        bpf_map_push_elem(&packet_map_template3, pinfo, BPF_ANY);
    } else if (hash_idx == 4) {
        bpf_map_push_elem(&packet_map_template4, pinfo, BPF_ANY);
    } else if (hash_idx == 5) {
        bpf_map_push_elem(&packet_map_template5, pinfo, BPF_ANY);
    } else if (hash_idx == 6) {
        bpf_map_push_elem(&packet_map_template6, pinfo, BPF_ANY);
    } else if (hash_idx == 7) {
        bpf_map_push_elem(&packet_map_template7, pinfo, BPF_ANY);
    } else if (hash_idx == 8) {
        bpf_map_push_elem(&packet_map_template8, pinfo, BPF_ANY);
    } else if (hash_idx == 9) {
        bpf_map_push_elem(&packet_map_template9, pinfo, BPF_ANY);
    }
}


SEC("prog")
int drop_all(struct __sk_buff *skb) {
    char delimiter[2];
    char redis_cmd[3];
    struct tcphdr tcp_header;
    __u64 offset = 1;

    struct packet_info pinfo = {};

    if (bpf_skb_load_bytes(skb, ETH_HLEN + sizeof(struct iphdr), &tcp_header, sizeof(tcp_header)) != 0){
        return -1;
    }



    __u32 tcp_hdr_len = tcp_header.doff*2;
    
    if(bpf_skb_load_bytes(skb, ETH_HLEN+sizeof(struct iphdr)+tcp_hdr_len, &redis_cmd,sizeof(redis_cmd)) !=0 ){
        return -1;
    }


    __u32 nhoff = skb->cb[0];
//    __be32 src = load_word(skb,nhoff + offsetof(struct iphdr, saddr));
//    __be32 dst = load_word(skb,nhoff + offsetof(struct iphdr, daddr));
    union port_t port;
    port.ports = load_word(skb,nhoff);

    bpf_printk("port : %u\n",port.port16[1]);

#if 1
    int hash_idx = find_index(port.port16[1]);
    bpf_printk("hash_idx : %d\n",hash_idx);


    if(bpf_memcmp(redis_cmd,"set",3)!=0 && bpf_memcmp(redis_cmd,"SET",3)!=0){
        if(bpf_memcmp(redis_cmd,"xxx",3)==0 || bpf_memcmp(redis_cmd,"XXX",3)==0)
            return -1;
        //GET 처리를 위해 사용     
        memset(pinfo.size, 0, sizeof(pinfo.size));
        bpf_push_elem(hash_idx, &pinfo);

        return -1;
    }
    offset = ETH_HLEN+sizeof(struct iphdr)+tcp_hdr_len+3; // set(3) \r\n(2) $3\r\n(4) key\r\n(5)
    
    int val_ptr;
    for (int j = 0; j < 4; ++j){
        for(int i=0; i<512; ++i){ // Assuming a maximum of 512 bytes to search
            if(bpf_skb_load_bytes(skb,offset+i, &delimiter,sizeof(delimiter))!=0)
              continue;
          if(bpf_memcmp(delimiter,"\r\n",2)==0){
              offset += i+2;
              if (j == 2){
                val_ptr = offset+1; //$제거 
             }
              break;
            }
        }
    }
    bpf_printk("22222222\n"); 
    bpf_skb_load_bytes(skb,val_ptr, &pinfo.size,sizeof(pinfo.size));
//    packet_size -= offset + 2; // 2(\r\n)
    offset -= (ETH_HLEN+sizeof(struct iphdr)-2);
    pinfo.offset = offset;

    bpf_push_elem(hash_idx, &pinfo);
#endif
    return -1;
}


char _license[] SEC("license") = "GPL";
