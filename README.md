# Interactive Tester

A lightweight, cross-platform CLI tool for debugging interactive problems in competitive programming (e.g., Codeforces, AtCoder, ICPC).

## Overview

In interactive competitive programming problems, your solution must communicate bi-directionally with a target interactor or checker binary over `stdin` and `stdout`. **Interactive Tester** launches both binaries as child processes and cross-pipes their standard input and output streams in real-time while monitoring execution time and exit codes.

```
┌─────────────────┐   stdout ──→ stdin   ┌─────────────────┐
│                 │                      │                 │
│   Contestant    │                      │     Checker     │
│    Solution     │                      │  / Interactor   │
│                 │   stdin  ←── stdout  │                 │
└─────────────────┘                      └─────────────────┘
```

---

## Features

- **Bi-Directional Piping**: Intercepts and routes outputs between solution and checker processes seamlessly.
- **Cross-Platform**: Works on Linux, macOS, and Windows.
- **Verbose Live Log**: Print timestamped communication between processes to `stdout` (`-v` / `--verbose`).
- **File Logging**: Save communication transcripts to a file (`-l` / `--log <file>`).
- **Timeout Enforcement**: Enforces a time limit in milliseconds and kills hanging processes (`-t` / `--timeout <ms>`).
- **Zero Heavy Dependencies**: Built with modern C++17, CMake, and bundled single-header [`argparse.hpp`](include/argparse.hpp).

---

## Installation

### Pre-built Packages (GitHub Releases)

Pre-compiled binary packages for each release are available on the [Releases](../../releases) page:

- **Windows (`.msi`)**: Download and run the installer wizard.
- **Debian / Ubuntu (`.deb`)**:
  ```bash
  sudo apt install ./interactive-tester-1.0.0-Linux.deb
  # or
  sudo dpkg -i interactive-tester-1.0.0-Linux.deb
  ```
- **Fedora / RHEL (`.rpm`)**:
  ```bash
  sudo dnf install ./interactive-tester-1.0.0-Linux.rpm
  # or
  sudo rpm -i interactive-tester-1.0.0-Linux.rpm
  ```
- **macOS (`.pkg`)**: Download and double-click the `.pkg` installer package to install.

### Building & Installing from Source

#### Prerequisites

- C++17 compliant compiler (GCC, Clang, or MSVC)
- CMake 3.14 or higher

#### Quick Commands (via Makefile)

On Unix-like systems, you can use the provided [Makefile](file:///home/stegg/Documents/cpp/interactive_tester/Makefile) for convenience:

```bash
make compile         # Build release binary in build/
make debug           # Build debug binary in build-debug/
make format          # Format codebase with clang-format
make format-check    # Verify formatting compliance
make test            # Compile test fixtures and run interactor integration test
make test-timeout    # Run timeout test case
sudo make install    # Install binary using CMake (supports PREFIX and DESTDIR)
sudo make uninstall  # Remove binary from /usr/local/bin
```

#### Building and Installing with CMake

```bash
# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel --config Release

# Install (optional)
sudo cmake --install build   # Supports --prefix <path>
```

On Windows (MSVC Command Prompt / PowerShell):

```cmd
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build
```

---

## Usage

```bash
interactive_tester <solution> <checker> [options]
```

### Positional Arguments

| Argument   | Description                              |
| ---------- | ---------------------------------------- |
| `solution` | Path to the contestant's solution binary |
| `checker`  | Path to the checker or interactor binary |

### Options & Flags

| Flag / Option | Short | Type   | Default | Description                               |
| ------------- | ----- | ------ | ------- | ----------------------------------------- |
| `--timeout`   | `-t`  | int    | `5000`  | Execution time limit in milliseconds      |
| `--verbose`   | `-v`  | flag   | `false` | Print process communication to `stdout`   |
| `--log`       | `-l`  | string | `""`    | Path to log file for saving communication |
| `--help`      | `-h`  | flag   | -       | Display help information                  |

---

## Examples

### 1. Basic Run

Run the solution and checker binaries:

```bash
./build/interactive_tester ./my_solution ./checker
```

**Output:**

```
[RESULT] Finished in 0.012s
Solution exit code: 0
Checker exit code:  0
```

### 2. Verbose Output to Console

Print all messages passed between processes with timestamps and direction indicators:

```bash
./build/interactive_tester ./my_solution ./checker -v
```

**Output:**

```
[0.002s] SOL -> CHK: 50
[0.002s] CHK -> SOL: >
[0.003s] SOL -> CHK: 25
[0.003s] CHK -> SOL: =

[RESULT] Finished in 0.004s
Solution exit code: 0
Checker exit code:  0
```

### 3. Log Communication to a File

Save transcript to `comm.log` while running:

```bash
./build/interactive_tester ./my_solution ./checker --log comm.log
```

### 4. Custom Time Limit (e.g. 2 Seconds)

Set a 2000 ms timeout:

```bash
./build/interactive_tester ./my_solution ./checker -t 2000 -v
```

---

## Project Structure

```
interactive_tester/
├── CMakeLists.txt              # CMake build configuration
├── SPECIFICATION.md            # Original project requirements
├── README.md                   # Documentation
├── include/
│   ├── argparse.hpp            # CLI argument parser (single-header)
│   ├── process.hpp             # Cross-platform process interface
│   └── interactive_tester.hpp  # Core orchestrator header
└── src/
    ├── main.cpp                # CLI entry point
    ├── interactive_tester.cpp  # Orchestrator implementation
    ├── process_unix.cpp        # POSIX process & pipe backend
    └── process_win.cpp         # Windows process & pipe backend
```
