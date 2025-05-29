cd BasicBPF && ./1_make_k.sh
cp packet_size_kern.o .. && cd ..
make USE_NVM=yes USE_BPF=yes -j
