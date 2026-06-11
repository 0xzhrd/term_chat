# Term Chat

**A simple terminal-based chat program using BSD sockets.**

Term Chat is a command-line application that enables real-time communication between a server and client over TCP sockets. The application features a terminal user interface with message history, file transfer capabilities, and raw terminal input handling.

## Description

Term Chat provides a lightweight solution for bidirectional communication through terminal interfaces. Built in C, it utilizes socket programming to establish reliable TCP connections. The application supports simultaneous message display and user input with support for file transfers up to 50 MB.

Key features include:

- **Client-server architecture** using TCP sockets
- **Real-time messaging** with asynchronous input and network event handling
- **File transfer protocol** for sharing files between clients and server
- **Terminal user interface** with raw input mode and cursor positioning
- **Thread-safe message buffering** using POSIX mutexes
- **Signal handling** for graceful shutdown and terminal state restoration

## Installation

### Prerequisites

- GCC compiler (gcc)
- POSIX-compliant operating system (Linux, macOS, BSD)
- Make build tool
- Standard C library with POSIX extensions

### Building from Source

Clone the repository and navigate to the project directory:

```bash
git clone https://github.com/0xzhrd/term_chat.git
cd term_chat
```

Compile the project using the provided Makefile:

```bash
make
```

This command will create a **build** directory containing compiled object files and generate the **chat** executable in the project root.

To clean up compiled files and the executable:

```bash
make clean
```

## Usage

### Starting the Server

Run the application in server mode on a specified port:

```bash
./chat server 5000
```

The server will listen for incoming client connections on port 5000.

### Connecting as a Client

In another terminal, connect to the server by providing the server IP address and port:

```bash
./chat client 127.0.0.1 5000
```

For remote connections, replace **127.0.0.1** with the server machine's IP address.

### In-Application Commands

**Send a message:** Type normally and press Enter to send.

**Send a file:** Use the command format:

```
::send /path/to/file
```

**Quit the application:** Type and send:

```
::quit
```

**Navigation:**

- **Page Up** Scroll up in message history
- **Page Down** Scroll down in message history
- **Arrow keys** Navigate cursor in input line

## Repository Structure

```
term_chat/
include/
  headers.h         Header file with common definitions and declarations
src/
  main.c            Entry point and signal handling
  server.c          Server connection handling
  client.c          Client connection handling
  transfers.c       File transfer protocol implementation
  terminal.c        Terminal interface and keyboard input
  logic.c           Message buffering and utility functions
build/              Compiled object files (generated)
README.md           This file
makefile            Build configuration
```

## Technical Details

The application uses **select()** for multiplexed I/O, enabling simultaneous handling of socket events and user keyboard input without blocking. Messages are stored in a thread-safe buffer protected by POSIX mutexes.

File transfers are handled through a custom protocol that sends file size and name metadata before transmitting file contents. The maximum file size is limited to 50 MB to prevent resource exhaustion.

Terminal state management preserves the original terminal configuration and restores it upon application exit, ensuring proper terminal behavior after program termination.
