# Copilot instructions

## Build, test, and lint

- Configure and build from a separate build directory:
  `cmake -S . -B build && cmake --build build --parallel`
- The executable is `build/interactive_tester` on Unix-like systems. On
  multi-configuration generators, build the Release configuration with
  `cmake --build build --config Release`.
- There is currently no CTest suite, registered test target, or lint target.
  The programs in `test/` are standalone communication fixtures, not unit
  tests. To exercise the example interaction on Unix-like systems, compile
  them and run the tester:

  ```bash
  c++ -std=c++17 test/echo_interactor.cpp -o build/echo_interactor
  c++ -std=c++17 test/guess_solution.cpp -o build/guess_solution
  build/interactive_tester build/guess_solution build/echo_interactor -v
  ```

- For a timeout case, compile `test/hang_solution.cpp` and run it as the
  solution with `-t`, for example:
  `build/interactive_tester build/hang_solution build/echo_interactor -t 100`.
- Formatting follows `.clang-format` (Google-based style, tabs, two-column
  indentation). No formatter is wired into CMake, so apply it explicitly
  when needed: `clang-format -i src/*.cpp include/*.hpp`.

## Architecture

- `src/main.cpp` is the CLI boundary. It parses two executable paths and the
  timeout, verbose, and log options with the bundled `include/argparse.hpp`,
  then constructs `InteractiveTester`.
- `InteractiveTester` in `src/interactive_tester.cpp` owns one `Process` for
  the contestant solution and one for the checker/interactor. Its loop
  non-blockingly reads each child’s stdout, forwards bytes to the other
  child’s stdin, buffers output into lines for logging, and stops when both
  processes finish or the shared timeout expires.
- `include/process.hpp` is the platform-neutral process/pipe API. The build
  selects exactly one backend in `CMakeLists.txt`: POSIX `fork`/`execv` and
  non-blocking file descriptors in `src/process_unix.cpp`, or Windows
  `CreateProcess` and inherited pipe handles in `src/process_win.cpp`.
- Child stderr is not part of the interaction transcript; the process
  backends inherit the parent’s standard error. The interaction protocol is
  stdout of one child to stdin of the other, in both directions.
- A normal run reports both child exit codes and returns nonzero if either
  child fails. A timeout terminates both children, reports time-limit
  exceeded, and returns `124`. Missing or unlaunchable binaries return `1`.

## Repository-specific conventions

- Keep public interfaces and shared data types in `include/`; put the
  orchestration implementation in `src/interactive_tester.cpp` and
  platform-specific process mechanics in the matching backend file.
- Preserve the two-child lifecycle and ownership rules in `Process`: it is
  non-copyable, movable, and responsible for closing pipe handles and
  terminating a still-running child during cleanup.
- Keep communication forwarding byte-oriented, but only emit verbose/log
  records after newline boundaries (and flush a final partial line at the
  end). Use `SOL` and `CHK` as the direction labels and retain timestamped
  records in the existing `[<seconds>s] SENDER -> RECEIVER: ...` shape.
- Flush interactive fixture output explicitly (`std::endl` or
  `std::flush`); buffering can otherwise make a correct interactive program
  appear hung and trigger the tester timeout.
- Add new CLI options in `main.cpp` and carry them through `TesterOptions`;
  keep default timeout behavior at 5000 ms unless the user-facing contract
  is intentionally changed.
- New source files must be added to `CMakeLists.txt`. Do not add the
  platform-specific process implementation to both builds; preserve the
  `WIN32` selection.
- Use the existing warning flags (`-Wall -Wextra -Wpedantic`) for GNU/Clang
  builds and keep code compatible with the project’s required C++17 standard.
- Treat executable paths as direct paths to binaries. The POSIX backend uses
  `execv` with no shell parsing, so shell commands, arguments, and pipelines
  are outside the tester’s CLI contract.
