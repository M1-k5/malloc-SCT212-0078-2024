# myalloc 
Title: Custom Memory Allocator
Name: Mike Owen Otieno
Student ID: SCT212-0078/2024

## Build & Run
```bash
make
./test_myalloc

Task Completion
1. Header & helpers	Complete
2. grow() with sbrk	Complete
3. First-fit malloc	Complete
4. Splitting	Complete
5. Free & coalescing	Partial
6. calloc/realloc	Complete
7. Best-fit/stats/dump/check	Complete
8. Benchmark & report	Complete

Explanation
The allocator relies on a single, singly-linked list to track all blocks in the heap in increasing address order. Each block has a header containing the payload size, a pointer to the next block, a free flag, and a magic number to detect invalid pointers.

When my_malloc is called, it rounds the requested size up to a multiple of 16 bytes. It scans the list using either First-Fit or Best-Fit (depending on the policy). If a suitable block is found, it is split if the remainder is large enough to hold a new header and a minimum payload (16 bytes). If no block is found, grow() is called to request a new 64KB chunk from the OS using sbrk(), aligning the payload to 16 bytes.

When my_free is called, the block is marked free. The allocator then scans the entire list to merge physically adjacent free blocks. This coalescing step is crucial for preventing external fragmentation. Because we only merge blocks if their memory addresses are perfectly contiguous, we avoid merging list neighbors that are separated by gaps from other memory allocations (like libc's internal malloc).

Questions
1.Why must the header size be a multiple of 16 as well as the payload sizes?
Because the payload address is calculated as header_address + HDR. If HDR is not a multiple of 16, then even if the payload size is a multiple of 16, the resulting payload address will be misaligned. This would cause undefined behavior on architectures that require strict alignment for certain data types.

2. Why can printf() inside my_malloc() cause infinite recursion?
The standard C library's printf() often uses malloc() internally to allocate buffers for formatting output. If our my_malloc() calls printf(), it triggers libc's malloc, which then calls our my_malloc() again, causing a stack overflow and infinite loop.

3. Your my_free() is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?
Adding a prev pointer to the block header (making it a doubly-linked list) would allow O(1) access to the previous block. The technique of storing both forward and backward pointers in the header is called boundary tags.

4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.
Internal fragmentation is wasted space inside an allocated block. In our allocator, rounding sizes up to a 16-byte boundary wastes space (e.g., requesting 1 byte allocates 16). External fragmentation is free memory that is split into small, non-contiguous chunks. In our allocator, external fragmentation can occur if we free many small blocks that cannot be merged because they are separated by allocated blocks.

5. Why does sbrk() memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.
Our grow() function only moves the program break upwards. To return memory, we would check if the last block in the list (the tail) is free and large enough. If so, we could call sbrk(-(HDR + tail->size)) to move the break backwards, and then unlink the block from our list.

6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?
This happens because libc's printf (used by the test harness) allocates memory using its own malloc, which moves the sbrk break and creates gaps between our chunks. Our coalescing code checks if payload(cur) + cur->size == (char*)cur->next before merging. If there is a gap, the pointers won't match, and we skip the merge.

Limitations
The coalescing logic has a minor edge case under heavy stress.
Memory is never returned to the OS via sbrk(-n).
No thread safety (no mutexes).

Sources & AI Declaration
I used online platforms like GeeksforGeeks and Linux man pages to understand memory alignment, sbrk, and block coalescing. I also used AI(Deepseek AI) for troubleshooting compiler errors and helping structure the design explanation, debug the coalescing loop and block splitting logic.
