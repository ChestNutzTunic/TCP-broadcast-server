# TCP-broadcast-server
Studying TCP/IP fundamentals, implementing a multithreaded server using win32 API and winsock. Developed to study socket programming, memory management, how to avoid race conditions, and cryptography basics.

As i enter to my second year as a CS student, i have developed a deep interest for networking. I started this project as a practical companion to my studies of 'TCP/IP Illustrated'. Even though Linux is the industry standard for most server-side applications, i chose to explore the Windows API to understand a little more how high-concurrency and synchronization primitives function within the NT Kernel architecture.

This project includes a simple RC4 based stream cipher with a substitution box for each client, ensuring secure communication, also using SRWlocks for optimized thread read/write usage, and implemented critical_section on the client-side to protect the encryption state during full-duplex communication.