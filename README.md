# Cache Memory Improvement
This project presents hardware techniques to improve the performance of caches. The processor lose half of their performance in the memory hierarchy if the hierarchy design is based in the conventional caching techniques. Using simplescalar we have implemented 3 different small fully associative caches and the results for the same were observed.

Implemented Strategies
======================
1. Victim Cache
2. Miss Cache
3. Selective Victim Cache

How to Use
==========

Copy and Paste these three files into simplesim directory
Build again and run
```
make
./sim-cache
```

Use following commands
======================
```
./sim-cache –cache<name> <option for cache> benchmark file <in file for benchmark>out file
victim cache
    -cache:dvictim dvictim:8:1 -cache:ivictim dvictim:8:1

miss cache
    -cache:dmcache dmcache:8:1 -cache:imcache dvictim:8:1

selective cache 
    -cache:dsel dsel:8:1 -cache:isel dsel:8:1
```

Example Commands
================
```
CC1 Benchmark ( Victim Cache )

./sim-cache -cache:dl1 dl1:64:64:1:l -cache:il1 il1:64:64:1:l -cache:dl2 dl2:2048:64:4:l cache:dvictim dvictim:8:l -cache:ivictim ivictim:8:l /home/ubuntu/harsh/little_endian_binaries/Little/cc1.ss </home/ubuntu/harsh/benchmarks/1stmt.i>OUT

CC1 Benchmark ( Miss Cache )

./sim-cache -cache:dl1 dl1:64:64:1:l -cache:il1 il1:64:64:1:l -cache:dl2 dl2:2048:64:4:l cache:dmcache dmcache:8:l -cache:imcache imcache:8:l /home/ubuntu/harsh/little_endian_binaries/Little/cc1.ss </home/ubuntu/harsh/benchmarks/1stmt.i>OUT

CC1 Benchmark ( Selective Victim Cache )

./sim-cache -cache:dl1 dl1:64:64:1:l -cache:il1 il1:64:64:1:l -cache:dl2 dl2:2048:64:4:l cache:dsel dsel:8:l -cache:isel isel:8:l /home/ubuntu/harsh/little_endian_binaries/Little/cc1.ss </home/ubuntu/harsh/benchmarks/1stmt.i>OUT
