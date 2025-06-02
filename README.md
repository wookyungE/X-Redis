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
   
      `make`
   
   -----------------------------------
   
   If you want to build the Pmem-Redis for DCPMM, if you didn't mount DCPMM as DAX:

      `여기 dax로 마운트 하는 코드`
   
      and then run command:
   
      `make USE_NVM=yes`

   -----------------------------------

   If you want to build the X-Redis, if you didn't mount DCPMM as DAX:

      `여기 dax로 마운트 하는 코드`
   
      and thenrun command:
   
      `make USE_NVM=yes, USE_BPF=yes`

4. Dependancies
   If you get an error when building, turn the next code and compound it again.

   `make distclean`

   You can change the options in redis.conf.
 
5. Run
   You can run X-Redis by the command:
   
   `./src/redis-server redis.conf`

   
