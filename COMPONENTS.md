# FC Embed Components Dependency Analysis

## Component Architecture

### 1. **Core** (Foundation Layer)
**Files:**
- `fc_compiler.h` - Compiler abstraction (ARM/IAR/GCC/MSVC)
- `fc_com.h` - Common definitions
- `fc_helper.h` - Helper macros
- `perfc_common.h` - Performance counter common definitions
- `fc_config_template.h` - User configuration template
- `fc_auto_init.c/h` - Auto-initialization framework

**Dependencies:** None (Base layer)

**Features:**
- Cross-compiler compatibility layer
- 4-stage initialization framework (ENV, CLOCK, DEVICE, APP)
- Priority-based initialization ordering
- Section-based automatic registration

---

### 2. **FIFO** (Header-Only)
**Files:**
- `fc_fifo.h` - Complete implementation

**Dependencies:** 
- Core (fc_compiler.h, fc_config.h)

**Features:**
- Lock-free ring buffer (single producer/consumer)
- Based on Linux kfifo design
- Power-of-2 size requirement
- Linear read/write operations
- Zero-copy operations support
- Optimized with bit masking instead of modulo

---

### 3. **Memory Pool**
**Files:**
- `fc_pool.c/h`

**Dependencies:**
- Core

**Features:**
- O(1) allocation/deallocation
- Fixed-size blocks
- Linked-chain support for non-contiguous memory
- Optional dynamic allocation support
- FIFO queue for used blocks
- Memory usage statistics

---

### 4. **Port Layer**
**Files:**
- `fc_port.c/h`

**Dependencies:**
- Core
- FIFO

**Features:**
- Multi-buffer I/O (up to 8 ring buffers per port)
- Similar to SEGGER RTT design
- Standard I/O redirection (stdout/stdin)
- Configurable buffer sizes
- Physical layer abstraction
- Lock-free with user-defined lock support

---

### 5. **Transport**
**Files:**
- `fc_trans.c/h`

**Dependencies:**
- FIFO
- Port

**Features:**
- Multiplexed channel transmission
- Uses ANSI escape sequences for channel switching (`\033[?;<num>m`)
- Sender and Receiver objects
- Up to 9999 virtual channels
- Single physical channel support

---

### 6. **Standard I/O**
**Files:**
- `fc_stdio.c/h`
- `utils/fc_sprintf.c`
- `utils/fc_snprintf.c`
- `utils/fc_vsprintf.c`
- `utils/fc_vsnprintf.c`
- `utils/fc_fprintf.c`
- `utils/fc_vfprintf.c` (core implementation)
- `utils/fc_port_vprintf.c` (requires Port)

**Dependencies:**
- Core
- (Optional) Port - for `fc_port_vprintf.c` functionality

**Features:**
- Based on xprintf design
- Printf family functions
- FC_FILE abstraction
- Format string support (d, u, x, X, b, s, c, f, E)
- Memory buffer or callback-based output

---

### 7. **Logger**
**Files:**
- `fc_log.c/h`
- `fc_rtt_helper.h`

**Dependencies:**
- Core
- Memory Pool (for buffer management)
- StdIO (for formatting)

**Features:**
- 5 log levels (ERROR, WARNING, INFO, DEBUG, VERBOSE)
- Color support via ANSI escape codes
- RTT integration ready
- Memory pool-based buffer management
- Line buffering
- Configurable prefix format
- Re-entrant macro API

---

## Dependency Graph

```
Core (Base)
├── FIFO (header-only)
│   └── Port
│       └── Transport
│           
├── Memory Pool
│   └── Logger (also requires StdIO)
│
└── StdIO
    └── Logger (also requires Memory Pool)
```

## Component Selection Guide

### Minimal Setup
- **Core** only: Compiler abstraction and auto-init

### Basic I/O
- Core + FIFO + Port: For buffered I/O operations

### Advanced I/O
- Core + FIFO + Port + Transport: For multiplexed channels

### Logging System
- Core + Memory Pool + StdIO + Logger: Complete logging

### Printf Only
- Core + StdIO: Printf family functions

## Configuration Requirements

### fc_config.h (or fc_config_template.h)
Users must provide a `fc_config.h` file with necessary definitions:

```c
// Example minimal configuration
#ifndef _FC_CONFIG_H_
#define _FC_CONFIG_H_

// FIFO settings
#define FC_USE_STD_MEMCPY 1

// Memory Pool settings
#define FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC 0

// Logger settings
#define FC_LOG_ENABLE 1
#define FC_LOG_LINE_SIZE 128
#define FC_LOG_POOL_TOTAL_SIZE (4 * 1024)

// Port settings
#define PORT_RB_NUM 8

#endif
```

## Key Features Summary

1. **Modular Design**: Each component can be used independently (with dependencies)
2. **Zero Dependencies**: Core layer has no external dependencies
3. **Header-Only Option**: FIFO can be used without compilation
4. **Cross-Platform**: Supports ARM, IAR, GCC, MSVC compilers
5. **Real-Time Ready**: Lock-free data structures where possible
6. **Memory Efficient**: Fixed-size pools, no dynamic allocation by default
7. **RTT Compatible**: Designed to work with SEGGER RTT ecosystem

## Build Notes

- FIFO is header-only, included files will handle compilation
- Utils in StdIO are included by `fc_stdio.c`, not compiled separately
- Auto-init uses constructor attribute on GCC, manual call needed on others
- Section-based registration requires linker script support

## Version

**Current Release:** 1.0.0 (2025-12-02)
