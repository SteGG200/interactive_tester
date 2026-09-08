#if defined(_WIN32) || defined(_WIN64)

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <utility>

#include "process.hpp"

Process::Process()
		: process_handle_(NULL),
			thread_handle_(NULL),
			stdin_write_(NULL),
			stdout_read_(NULL),
			running_(false),
			stdin_closed_(false),
			stdout_closed_(false),
			exit_code_(-1) {}

Process::~Process() { cleanup(); }

Process::Process(Process&& other) noexcept
		: process_handle_(other.process_handle_),
			thread_handle_(other.thread_handle_),
			stdin_write_(other.stdin_write_),
			stdout_read_(other.stdout_read_),
			running_(other.running_),
			stdin_closed_(other.stdin_closed_),
			stdout_closed_(other.stdout_closed_),
			exit_code_(other.exit_code_) {
	other.process_handle_ = NULL;
	other.thread_handle_ = NULL;
	other.stdin_write_ = NULL;
	other.stdout_read_ = NULL;
	other.running_ = false;
}

Process& Process::operator=(Process&& other) noexcept {
	if (this != &other) {
		cleanup();
		process_handle_ = other.process_handle_;
		thread_handle_ = other.thread_handle_;
		stdin_write_ = other.stdin_write_;
		stdout_read_ = other.stdout_read_;
		running_ = other.running_;
		stdin_closed_ = other.stdin_closed_;
		stdout_closed_ = other.stdout_closed_;
		exit_code_ = other.exit_code_;

		other.process_handle_ = NULL;
		other.thread_handle_ = NULL;
		other.stdin_write_ = NULL;
		other.stdout_read_ = NULL;
		other.running_ = false;
	}
	return *this;
}

bool Process::launch(const std::string& executable_path) {
	cleanup();

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	HANDLE child_stdout_write = NULL;
	HANDLE child_stdin_read = NULL;

	// Create stdout pipe
	if (!CreatePipe(&stdout_read_, &child_stdout_write, &saAttr, 0)) {
		return false;
	}
	// Ensure the read handle to the pipe for STDOUT is not inherited
	if (!SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(stdout_read_);
		CloseHandle(child_stdout_write);
		return false;
	}

	// Create stdin pipe
	if (!CreatePipe(&child_stdin_read, &stdin_write_, &saAttr, 0)) {
		CloseHandle(stdout_read_);
		CloseHandle(child_stdout_write);
		return false;
	}
	// Ensure the write handle to the pipe for STDIN is not inherited
	if (!SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(stdout_read_);
		CloseHandle(child_stdout_write);
		CloseHandle(child_stdin_read);
		CloseHandle(stdin_write_);
		return false;
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOA));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	si.cb = sizeof(STARTUPINFOA);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = child_stdout_write;
	si.hStdInput = child_stdin_read;
	si.dwFlags |= STARTF_USESTDHANDLES;

	std::string cmd = executable_path;
	BOOL success = CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL,
																NULL, TRUE, 0, NULL, NULL, &si, &pi);

	// Parent closes child's handles
	CloseHandle(child_stdout_write);
	CloseHandle(child_stdin_read);

	if (!success) {
		CloseHandle(stdout_read_);
		CloseHandle(stdin_write_);
		stdout_read_ = NULL;
		stdin_write_ = NULL;
		return false;
	}

	process_handle_ = pi.hProcess;
	thread_handle_ = pi.hThread;
	running_ = true;
	stdin_closed_ = false;
	stdout_closed_ = false;
	exit_code_ = -1;

	return true;
}

int Process::read_stdout(char* buffer, size_t max_bytes, bool& eof) {
	eof = false;
	if (stdout_read_ == NULL || stdout_closed_) {
		eof = true;
		return -1;
	}

	DWORD bytes_avail = 0;
	if (!PeekNamedPipe(stdout_read_, NULL, 0, NULL, &bytes_avail, NULL)) {
		DWORD err = GetLastError();
		if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
			eof = true;
			return 0;
		}
		eof = true;
		return -1;
	}

	if (bytes_avail == 0) {
		// Check if process has terminated and pipe is broken
		if (!is_running()) {
			// Check one more peek
			if (!PeekNamedPipe(stdout_read_, NULL, 0, NULL, &bytes_avail, NULL) ||
					bytes_avail == 0) {
				eof = true;
				return 0;
			}
		} else {
			return 0;
		}
	}

	DWORD bytes_to_read =
			static_cast<DWORD>(std::min(max_bytes, static_cast<size_t>(bytes_avail)));
	DWORD bytes_read = 0;
	if (ReadFile(stdout_read_, buffer, bytes_to_read, &bytes_read, NULL)) {
		if (bytes_read == 0) {
			eof = true;
		}
		return static_cast<int>(bytes_read);
	} else {
		DWORD err = GetLastError();
		if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
			eof = true;
			return 0;
		}
		eof = true;
		return -1;
	}
}

bool Process::write_stdin(const char* data, size_t size) {
	if (stdin_write_ == NULL || stdin_closed_) {
		return false;
	}

	DWORD total_written = 0;
	while (total_written < size) {
		DWORD written = 0;
		BOOL success =
				WriteFile(stdin_write_, data + total_written,
									static_cast<DWORD>(size - total_written), &written, NULL);

		if (!success) {
			return false;
		}
		total_written += written;
	}
	return true;
}

void Process::close_stdin() {
	if (stdin_write_ != NULL && !stdin_closed_) {
		CloseHandle(stdin_write_);
		stdin_write_ = NULL;
		stdin_closed_ = true;
	}
}

void Process::close_stdout() {
	if (stdout_read_ != NULL && !stdout_closed_) {
		CloseHandle(stdout_read_);
		stdout_read_ = NULL;
		stdout_closed_ = true;
	}
}

bool Process::is_running() {
	if (!running_ || process_handle_ == NULL) {
		return false;
	}

	DWORD res = WaitForSingleObject(process_handle_, 0);
	if (res == WAIT_OBJECT_0) {
		DWORD exit_code = 0;
		if (GetExitCodeProcess(process_handle_, &exit_code)) {
			exit_code_ = static_cast<int>(exit_code);
		} else {
			exit_code_ = 1;
		}
		running_ = false;
		return false;
	}
	return true;
}

int Process::wait(int timeout_ms) {
	if (!running_ || process_handle_ == NULL) {
		return exit_code_;
	}

	DWORD res =
			WaitForSingleObject(process_handle_, static_cast<DWORD>(timeout_ms));
	if (res == WAIT_OBJECT_0) {
		DWORD exit_code = 0;
		if (GetExitCodeProcess(process_handle_, &exit_code)) {
			exit_code_ = static_cast<int>(exit_code);
		} else {
			exit_code_ = 1;
		}
		running_ = false;
		return exit_code_;
	}

	return -1;	// Timeout
}

void Process::terminate() {
	if (running_ && process_handle_ != NULL) {
		TerminateProcess(process_handle_, 1);
		exit_code_ = 1;
		running_ = false;
	}
	cleanup();
}

void Process::cleanup() {
	close_stdin();
	close_stdout();
	if (thread_handle_ != NULL) {
		CloseHandle(thread_handle_);
		thread_handle_ = NULL;
	}
	if (process_handle_ != NULL) {
		CloseHandle(process_handle_);
		process_handle_ = NULL;
	}
	running_ = false;
}

#endif	// defined(_WIN32) || defined(_WIN64)
