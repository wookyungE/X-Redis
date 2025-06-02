## What is X-Redis?
X-Redis is an extended version of Pmem-Redis utilizing eBPF.

## PPF
PPF(Packet Peek and forward) is a framework that peeks at the packets in the kernel layer with eBPF and streamlines data placement decisions on tiered memory.

## OMA
OMA(Opportune Memory Allocator) is that moves zeroing off the critical path in DAX filesystem.

## Getting Start
1. clone the code
   git clone https://github.com/wookyungE/X-Redis
2. compile
   If you want to build the original opensource redis-4.0.0, run command:
   
      make
   
   If you want to build the Pmem-Redis for DCPMM, run command:
   
      make USE_NVM=yes
