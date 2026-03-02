# TCP-broadcast-server
Studying TCP/IP fundamentals, implementing a multithreaded server using win32 API and winsock. Developed to study socket programming, memory management, race conditions mitigation, and cryptography basics.

As i enter to my second year as a CS student, i have developed a deep interest for networking. I started this project as a practical companion to my studies of 'TCP/IP Illustrated'. Even though Linux is the industry standard for most server-side applications, i chose to explore the Windows API to understand a little more how high-concurrency and synchronization primitives function within the NT Kernel architecture.

This project includes a simple RC4 based stream cipher with a substitution box for each client, ensuring secure communication. It also utilizes SRWLocks for optimized thread read/write usage and implements Critical Sections on the client-side to protect the encryption state during full-duplex communication.
Initially, my idea was to create a thread for each new client to run a basic communication function. However, it became clear that this would waste a significant amount of memory. Instead, I implemented a thread pool with IOCP (Input/Output Completion Ports) to handle WSASend() and WSArecv() operations with minimal performance deterioration.

Things that i would like to implement furthermore in this project:
-A buffer pool to minimize malloc operations;
-A timer to identify dead-connections, inactive users, etc. Mitigating memory leak;
-Implementing server-side commands;
-Implementing file transfer, making it possible to send photos, videos, and any other file type;
-Checking how many pending messages a client has, and either stop sending anything, or disconnect him for excessive lag, as it would consume unnecessary RAM;