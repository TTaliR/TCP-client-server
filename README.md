# TCP Socket Communication in C

This project implements a TCP client-server architecture using C sockets. The client connects to the server, sends messages, and receives server responses in a blocking (synchronous) manner. This exercise demonstrates core concepts of socket programming in C.

## 📂 Files

- `TCP_Client.c` – Implements the client-side application
- `TCP_BlockingServer.c` – Implements a blocking TCP server
- `Sockets.txt` – Project instructions and notes
- `EX2_315489856_206540007.docx` – Project documentation

## 🚀 How to Compile and Run

### Server

```bash
gcc TCP_BlockingServer.c -o server
./server <port>
```

### Client

```bash
gcc TCP_Client.c -o client
./client <server_ip> <port>
```

## 🔧 Features

- TCP socket setup (bind, listen, accept)
- Blocking communication model
- Message echo and basic handling

## 📄 Requirements

- GCC compiler
- Linux/Unix-based OS (for sockets API compatibility)
- Terminal access

## 📘 Notes

This project was completed as part of a university assignment on network programming.

---

## 📄 License

This project is for educational use only.
