# TCP-broadcast-server

## Technical Overview

Studying TCP/IP fundamentals, implementing a multithreaded server using win32 API and winsock. Developed to study socket programming, memory management, race conditions mitigation, and cryptography basics.

As i enter to my second year as a CS student, i have developed a deep interest for networking. I started this project as a practical companion to my studies of 'TCP/IP Illustrated'. Even though Linux is the industry standard for most server-side applications, i chose to explore the Windows API to understand a little more how high-concurrency and synchronization primitives function within the NT Kernel architecture.

Initially, my idea was to create a thread for each new client to run a basic communication function. However, it became clear that this would waste a significant amount of memory. Instead, I implemented a thread pool with IOCP (Input/Output Completion Ports) to handle WSASend() and WSArecv() operations with minimal performance deterioration.
This was the first project that made me go as far as learning x86_64 assembly to implement a function, simply for learning experience and maximum performance, it was very challenging and fun at the same time. 

### This project includes:
- A simple RC4 based stream cipher with a substitution box for each client, ensuring secure communication;
- SRWLocks for optimized thread read/write usage and Critical Sections on the client-side to protect the encryption state during full-duplex communication;
- Reference Counting for Completion Ports memory management, more specifically, to manage the CLIENT object life cycle;
- A Lock-Free Buffer Arena to eliminate the overhead of malloc/free. It uses a singly linked list, completely inspired by sList_Header, a known and very used structure given by the WIN32 API;
- Custom x86_64 Assembly: Since standard atomic intrinsics can be restrictive, i wasn't able to use the function that was necessary for atomic compare/exchange of the linked list header, i wrote a custom _InterlockedCompareExchange128 (using lock cmpxchg16b) in NASM to handle the ABA problem;

### Things that i would like to implement furthermore in this project:
- A timer to identify dead-connections, inactive users, etc. Mitigating memory leak;
- Implementing server-side commands;
- Implementing file transfer;
- Checking how many pending messages a client has, and either stop sending anything, or disconnect him for excessive lag, as it would consume unnecessary RAM;