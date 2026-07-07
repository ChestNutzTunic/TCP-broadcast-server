# TCP-broadcast-server

## Technical Overview

Studying TCP/IP fundamentals, implementing a multithreaded server using win32 API and winsock. Developed to study socket programming, memory management, race conditions mitigation, and cryptography basics.

I started this project as a practical companion to my studies of 'TCP/IP Illustrated'. Even though Linux is the industry standard for most server-side applications, i chose to explore the Windows API to understand a little more how high-concurrency and synchronization primitives function within the NT Kernel architecture.

Initially, my idea was to create a thread for each new client to run a basic communication function. However, it became clear that this would waste a significant amount of memory. Instead, I implemented a thread pool with IOCP (Input/Output Completion Ports) to handle WSASend() and WSArecv() operations with minimal performance deterioration.
Implementing x86 Assembly was challenging, but also very rewarding.
Also, it's important to note that usually the threshold for both the amount of messages that can be sent and received would be much smaller, since no human can read thousand of messages appearing simultaneously, but i was experimenting with the server efficiency, so i just left it as is.

## This project includes:
- A simple RC4 based stream cipher with a substitution box for each client, ensuring secure communication;
- SRWLocks for optimized thread read/write usage and Critical Sections on the client-side to protect the encryption state during full-duplex communication;
- Reference Counting for Completion Ports memory management, more specifically, to manage the CLIENT object life cycle;
- A lock-free buffer arena using a free list (inspired by slist_header) and linear backoff to reduce CAS(Compare and Swap) contention under high thread concurrency;
- Custom x86_64 Assembly: Since standard atomic intrinsics can be restrictive, i wasn't able to use the function that was necessary for atomic compare/exchange of the linked list header, i wrote a custom _InterlockedCompareExchange128 (using lock cmpxchg16b) to handle the ABA problem;
- Use of a reference counter to not only know when to free communication info that is not useful anymore, but also to constantly check if the users are spamming or not, and if so, ban them to mitigate excess memory usage.

## Things that i would like to implement furthermore in this project:
- A pseudo-random key generator for encrypting data;

## How to compile
### Compiling the cmpxchg.asm
To compile the x86 assembly, you must first install nasm.
Then, use:
```bash
nasm -f win64 cmpxchg16.asm -o cmpxchg16.o
```

### Compiling everything together
To compile everything, after already compiling the assembly file, use:
```bash
gcc -g server.c dyn_arr.c crypto.c arena.c cmpxchg16.o -o server -lws2_32 -lpsapi 
```
Without -lws2_32, you won't be able to link ws2_32.dll, and by doing so, you will not be able to use any socket-based functionality.
And without -lpsapi, you won't be able to link psapi.dll, therefore, you will not be able to use any process-status-api functionality.
