# Benchmark and Report
Tested the allocator with a random sequence of 50,000 allocations and frees, with sizes ranging from 1 to 4096 bytes using a fixed seed (srand(42)).

### Results

 Policy ---- Time in seconds ------ Heap Bytes ----- Free Bytes ------ Free Blocks ---- Largest Free --- External Fragmentation 
 First Fit ---- 0.0452 ---- 131072 ---- 85040 ---- 12 ---- 60024 --- 0.294 
 Best Fit ---- 0.0515 ---- 131072 ---- 85040 ---- 48 ---- 12050 --- 0.858 

### Discussion

Based on the results, I would choose the First Fit policy for this workload. First Fit is faster and has an obvious lower external fragmentation as compared to Best Fit. Best Fit tends to create many tiny, unusable free blocks by continuously shaving off exact sizes, leading to a highly fragmented heap. First Fit scans quickly and leaves larger contiguous chunks available.

Our allocator is slower and less space-efficient than glibc's malloc primarily because we use a simple singly-linked list. Finding a free block requires an O(n) scan of the entire list. In addition, we do not return memory to the OS via sbrk(-n), and our strict 16-byte header alignment adds internal fragmentation overhead compared to highly optimized production allocators that use binning and boundary tags.
