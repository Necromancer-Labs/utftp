# Ultra TFTP

```
  █▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀█
  █        Ultra TFTP         █
  █       Fast. Simple.       █
  █▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█

14:32:01 [INF] Listening on 0.0.0.0:69
14:32:01 [INF] Serving from /srv/tftp
14:32:01 [INF] Ready for connections (max 64 concurrent)
14:32:15 [INF] <-- GET firmware.bin (4.2 MB) from 192.168.1.100:54321
14:32:18 [INF] SENT SUCCESS firmware.bin 4.2 MB @ 1.4 MB/s to 192.168.1.100:54321
14:32:22 [INF] --> PUT config.tar from 192.168.1.100:54322
14:32:23 [INF] RECV SUCCESS config.tar 156.3 KB @ 312.6 KB/s from 192.168.1.100:54322
```

<p align="center">
  <img src="https://img.shields.io/badge/Necromancer_Labs-Tools-8A2BE2?style=for-the-badge&labelColor=374151" alt="Necromancer Labs">
  <img src="https://img.shields.io/badge/Focus-Networking-8A2BE2?style=for-the-badge&labelColor=374151" alt="Focus">
  <img src="https://img.shields.io/badge/Status-Active_Dev-22c55e?style=for-the-badge&labelColor=374151" alt="Status">
  <br>
  <a href="https://github.com/Necromancer-Labs"><img src="https://img.shields.io/badge/GitHub-Necromancer--Labs-8A2BE2?style=for-the-badge&labelColor=374151&logo=github&logoColor=white" alt="GitHub"></a>
</p>

A lightweight, high-performance TFTP server written in C. 
Designed to be fast and simple. 
Perfect for embedded systems for firmware flashing/pulling, embedded device provisioning, network booting, etc.

## Usage

```
Ultra TFTP Server

Usage: ./utftp [options]

Options:
  -i, --ip ADDR       Bind to specific IP address (default: 0.0.0.0)
  -p, --port PORT     Listen port (default: 69)
  -r, --root DIR      Root directory (default: current)
  -t, --timeout SEC   Timeout in seconds (default: 30)
  -d, --debug         Enable debug logging
  -q, --quiet         Quiet mode (critical errors only)
  -h, --help          Show this help
```

## Features

- **Zero Dependencies** - Pure C, compiles to a single static binary
- **RFC Compliant** - Full implementation of TFTP standards (see below)
- **Secure** - Path traversal protection prevents directory escape attacks
- **Beautiful Logging** - Color-coded output with transfer speeds and file sizes

## Download latest release

## If you want to build it yourself for x86

```bash
# Build
make

# Run (requires root for port 69, or use -p for alternate port)
sudo ./utftp -r /path/to/files

# Test on high port (no root needed)
./utftp -p 6969 -r ./files
```

---

## RFC Implementation

Ultra TFTP implements the TFTP protocol standards with careful attention to the original specifications:

| RFC | Title | Status |
|-----|-------|--------|
| [RFC 1350](https://tools.ietf.org/html/rfc1350) | The TFTP Protocol (Revision 2) | ✅ Full |
| [RFC 2347](https://tools.ietf.org/html/rfc2347) | TFTP Option Extension | ✅ Full |
| [RFC 2348](https://tools.ietf.org/html/rfc2348) | TFTP Blocksize Option | ✅ Full |
| [RFC 2349](https://tools.ietf.org/html/rfc2349) | TFTP Timeout & Transfer Size Options | ✅ tsize supported |

### Supported Opcodes

| Opcode | Name | Description |
|--------|------|-------------|
| 1 | RRQ | Read Request - Client wants to download a file |
| 2 | WRQ | Write Request - Client wants to upload a file |
| 3 | DATA | Data packet with payload (up to blksize bytes) |
| 4 | ACK | Acknowledgment of received data block |
| 5 | ERROR | Error notification with code and message |
| 6 | OACK | Options Acknowledgment for negotiated parameters |

### Supported Options (RFC 2347/2348/2349)

- **blksize** - Block size negotiation (8 to 65464 bytes)
- **tsize** - Transfer size reporting

### Transfer Modes

- **octet** - Binary mode (recommended)
- **netascii** - Text mode (treated as binary for compatibility)

---

## ⚠️ Understanding Transfer Speeds

**This is important if you're working with embedded devices!**

```
┌─────────────────────────────────────────────────────────────────────┐
│                    TFTP BLOCK SIZE NEGOTIATION                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌──────────────┐         REQUEST          ┌──────────────────┐    │
│   │    CLIENT    │ ──────────────────────── │  SERVER (utftp)  │    │
│   │  (Device)    │         blksize?         │                  │    │
│   └──────────────┘                          └──────────────────┘    │
│          │                                          │               │
│          │     If client supports RFC 2348:         │               │
│          │     ◄──────── OACK (blksize=X) ──────────│               │
│          │     Fast transfers with large blocks!    │               │
│          │                                          │               │
│          │     If client is BusyBox/embedded:       │               │
│          │     ◄──────── DATA (512 bytes) ──────────│               │
│          │     Limited to 512-byte blocks :(        │               │
│          ▼                                          ▼               │
└─────────────────────────────────────────────────────────────────────┘
```

### The Reality of Embedded Devices

The TFTP protocol (RFC 1350) uses a **default block size of 512 bytes**. While Ultra TFTP fully supports block size negotiation (to go faster) through the `blksize` option (RFC 2348), the **client** must also support this option for larger blocks to be used (during negociation). If the device/router/etc does NOT support that option (for example a lot of busybox binaries do not support beyond 512 bytes) then it defaults to 512. 

### What This Means

| Scenario | Block Size | Throughput |
|----------|------------|------------|
| Modern client with RFC 2348 | Up to 65464 bytes | 🚀 Fast |
| BusyBox/embedded client | 512 bytes | 🐌 Slow |

**The transfer speed is determined by the TARGET device, not this server.**

If you're pushing firmware to a router or NVR and transfers seem slow, it's because the device's TFTP client doesn't support block size negotiation. This is a limitation of the target device, not Ultra TFTP.

### Testing Block Size Support

```bash
# Modern tftp client (supports blksize)
tftp -m binary -c get firmware.bin

# Check if your target supports blksize by watching server logs
# If you see OACK in debug mode, negotiation succeeded
./utftp -d -p 6969 -r ./files
```

---

## Custom Modifications

This implementation includes several enhancements over standard TFTP servers:

| Change | Description |
|--------|-------------|
| **Extended Timeout** | Default timeout increased from 5 to 30 seconds for reliability with slow/unreliable links |
| **Configurable Timeout** | `-t` flag allows custom timeout values for specific use cases |
| **Standalone Binary** | Compiles to a single executable with no runtime dependencies |
| **Color Output** | Auto-detected terminal colors for better visibility |
| **Transfer Metrics** | Real-time speed and size reporting on completion |

---

## Building

```bash
# Standard optimized build
make

# Debug build with AddressSanitizer
make debug

# Fully static binary (portable across systems)
make static

# Install to /usr/local/bin
sudo make install

# Clean build artifacts
make clean
```

## Project Structure

```
utftp/
├── include/
│   ├── utftp.h      # Types, constants, structs
│   ├── log.h        # Logging functions
│   ├── packet.h     # Packet encode/decode
│   ├── server.h     # Server lifecycle
│   ├── session.h    # Session management
│   ├── transfer.h   # Transfer handlers
│   └── util.h       # Utilities
├── src/
│   ├── main.c       # Entry point, CLI
│   ├── server.c     # Server core
│   ├── session.c    # Session management
│   ├── transfer.c   # RRQ/WRQ/ACK/DATA
│   ├── packet.c     # Packet building
│   ├── log.c        # Colored logging
│   └── util.c       # Path security
├── Makefile
└── README.md
```

## Security Notes

- **Path Traversal Protection**: All file paths are validated to prevent `../` escape attacks
- **Root Directory Jail**: Files are served only from the configured root directory
- **No Shell Execution**: Pure file I/O, no command execution
- **Privilege Dropping**: Consider running behind a reverse proxy or with dropped privileges after binding

## License

MIT License

---

<p align="center">
  <b>Fast. Simple. Ready.</b><br>
  Built for those who need files moved, fast.
</p>
