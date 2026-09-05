#if !defined(_WIN32) && !defined(_WIN64)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <utility>

#include "process.hpp"

Process::Process()
		: pid_(-1),
			stdin_write_(-1),
			stdout_read_(-1),
			running_(false),
			stdin_closed_(false),
			stdout_closed_(false),
			exit_code_(-1) {
	// Ignore SIGPIPE so write() to closed pipe returns -1 instead of killing
	// parent process
	signal(SIGPIPE, SIG_IGN);
}

Process::~Process() { cleanup(); }

Process::Process(Process&& other) noexcept
		: pid_(other.pid_),
			stdin_write_(other.stdin_write_),
			stdout_read_(other.stdout_read_),
			running_(other.running_),
			stdin_closed_(other.stdin_closed_),
			stdout_closed_(other.stdout_closed_),
			exit_code_(other.exit_code_) {
	other.pid_ = -1;
	other.stdin_write_ = -1;
	other.stdout_read_ = -1;
	other.running_ = false;
}

Process& Process::operator=(Process&& other) noexcept {
	if (this != &other) {
		cleanup();
		pid_ = other.pid_;
		stdin_write_ = other.stdin_write_;
		stdout_read_ = other.stdout_read_;
		running_ = other.running_;
		stdin_closed_ = other.stdin_closed_;
		stdout_closed_ = other.stdout_closed_;
		exit_code_ = other.exit_code_;

		other.pid_ = -1;
		other.stdin_write_ = -1;
		other.stdout_read_ = -1;
		other.running_ = false;
	}
	return *this;
}

bool Process::launch(const std::string& executable_path) {
	cleanup();

	int pipe_stdin[2] = {
			-1, -1};	// pipe_stdin[0] = child read, pipe_stdin[1] = parent write
	int pipe_stdout[2] = {
			-1, -1};	// pipe_stdout[0] = parent read, pipe_stdout[1] = child write

	if (pipe(pipe_stdin) < 0) {
		return false;
	}
	if (pipe(pipe_stdout) < 0) {
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		return false;
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		return false;
	}

	if (pid == 0) {
		// Child process
		dup2(pipe_stdin[0], STDIN_FILENO);
		dup2(pipe_stdout[1], STDOUT_FILENO);

		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);

		char* const args[] = {const_cast<char*>(executable_path.c_str()), nullptr};
		execv(executable_path.c_str(), args);
		// If execv fails
		_exit(127);
	}

	// Parent process
	pid_ = pid;
	close(pipe_stdin[0]);		// close child's read end
	close(pipe_stdout[1]);	// close child's write end

	stdin_write_ = pipe_stdin[1];
	stdout_read_ = pipe_stdout[0];

	// Set parent's read end (child's stdout) to non-blocking
	int flags = fcntl(stdout_read_, F_GETFL, 0);
	if (flags != -1) {
		fcntl(stdout_read_, F_SETFL, flags | O_NONBLOCK);
	}

	// Set parent's write end (child's stdin) to non-blocking as well
	flags = fcntl(stdin_write_, F_GETFL, 0);
	if (flags != -1) {
		fcntl(stdin_write_, F_SETFL, flags | O_NONBLOCK);
	}

	running_ = true;
	stdin_closed_ = false;
	stdout_closed_ = false;
	exit_code_ = -1;

	return true;
}

int Process::read_stdout(char* buffer, size_t max_bytes, bool& eof) {
	eof = false;
	if (stdout_read_ < 0 || stdout_closed_) {
		eof = true;
		return -1;
	}

	ssize_t bytes_read = read(stdout_read_, buffer, max_bytes);
	if (bytes_read > 0) {
		return static_cast<int>(bytes_read);
	} else if (bytes_read == 0) {
		eof = true;
		return 0;
	} else {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		eof = true;
		return -1;
	}
}

bool Process::write_stdin(const char* data, size_t size) {
	if (stdin_write_ < 0 || stdin_closed_) {
		return false;
	}

	size_t total_written = 0;
	while (total_written < size) {
		ssize_t written =
				write(stdin_write_, data + total_written, size - total_written);
		if (written < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(1000);
				continue;
			}
			return false;
		}
		total_written += written;
	}
	return true;
}

void Process::close_stdin() {
	if (stdin_write_ >= 0 && !stdin_closed_) {
		close(stdin_write_);
		stdin_write_ = -1;
		stdin_closed_ = true;
	}
}

void Process::close_stdout() {
	if (stdout_read_ >= 0 && !stdout_closed_) {
		close(stdout_read_);
		stdout_read_ = -1;
		stdout_closed_ = true;
	}
}

bool Process::is_running() {
	if (!running_ || pid_ <= 0) {
		return false;
	}

	int status = 0;
	pid_t res = waitpid(pid_, &status, WNOHANG);
	if (res == pid_) {
		running_ = false;
		if (WIFEXITED(status)) {
			exit_code_ = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			exit_code_ = 128 + WTERMSIG(status);
		} else {
			exit_code_ = 1;
		}
		return false;
	}
	return res == 0;
}

int Process::wait(int timeout_ms) {
	if (!running_) {
		return exit_code_;
	}

	int elapsed = 0;
	const int step_ms = 5;
	while (elapsed < timeout_ms) {
		if (!is_running()) {
			return exit_code_;
		}
		usleep(step_ms * 1000);
		elapsed += step_ms;
	}

	if (!is_running()) {
		return exit_code_;
	}
	return -1;	// Timeout
}

void Process::terminate() {
	if (running_ && pid_ > 0) {
		kill(pid_, SIGKILL);
		int status = 0;
		waitpid(pid_, &status, 0);
		running_ = false;
		exit_code_ = 137;
	}
	cleanup();
}

void Process::cleanup() {
	close_stdin();
	close_stdout();
	if (running_ && pid_ > 0) {
		terminate();
	}
	pid_ = -1;
	running_ = false;
}

#endif	// !defined(_WIN32) && !defined(_WIN64)
