#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
using NativeHandle = HANDLE;
#else
using NativeHandle = int;
#endif

class Process {
 public:
	Process();
	~Process();

	Process(const Process&) = delete;
	Process& operator=(const Process&) = delete;

	Process(Process&& other) noexcept;
	Process& operator=(Process&& other) noexcept;

	// Launch an executable given its file path
	bool launch(const std::string& executable_path);

	// Non-blocking read from child stdout.
	// Returns number of bytes read (0 if no data currently available).
	// Sets eof = true if the pipe has been closed / EOF reached.
	int read_stdout(char* buffer, size_t max_bytes, bool& eof);

	// Write data to child stdin.
	bool write_stdin(const char* data, size_t size);

	// Close parent's write end of child stdin (sends EOF to child)
	void close_stdin();

	// Close parent's read end of child stdout
	void close_stdout();

	// Check if child is still running
	bool is_running();

	// Wait for child exit up to timeout_ms.
	// Returns exit code (>= 0) if exited, or -1 if timed out / failed.
	int wait(int timeout_ms);

	// Force terminate child process
	void terminate();

	NativeHandle get_read_handle() const { return stdout_read_; }
	NativeHandle get_write_handle() const { return stdin_write_; }

 private:
	void cleanup();

#if defined(_WIN32) || defined(_WIN64)
	HANDLE process_handle_ = NULL;
	HANDLE thread_handle_ = NULL;
	HANDLE stdin_write_ = NULL;
	HANDLE stdout_read_ = NULL;
#else
	int pid_ = -1;
	int stdin_write_ = -1;
	int stdout_read_ = -1;
#endif
	bool running_ = false;
	bool stdin_closed_ = false;
	bool stdout_closed_ = false;
	int exit_code_ = -1;
};

#endif	// PROCESS_HPP
