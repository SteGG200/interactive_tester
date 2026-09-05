#include "interactive_tester.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "process.hpp"

namespace fs = std::filesystem;

InteractiveTester::InteractiveTester(const TesterOptions& options)
		: options_(options) {
	if (!options_.log_path.empty()) {
		log_file_.open(options_.log_path, std::ios::out | std::ios::trunc);
		if (!log_file_.is_open()) {
			std::cerr << "Warning: Could not open log file: " << options_.log_path
								<< std::endl;
		}
	}
}

InteractiveTester::~InteractiveTester() {
	if (log_file_.is_open()) {
		log_file_.close();
	}
}

std::string InteractiveTester::sanitize(const std::string& input) {
	std::string clean;
	clean.reserve(input.size());
	for (char c : input) {
		if (c == '\n' || c == '\r' || c == '\t' ||
				(static_cast<unsigned char>(c) >= 32 &&
				 static_cast<unsigned char>(c) <= 126)) {
			clean.push_back(c);
		} else {
			clean.push_back('.');
		}
	}
	return clean;
}

void InteractiveTester::log_message(const std::string& sender,
																		const std::string& receiver,
																		const std::string& text,
																		double elapsed_sec) {
	std::ostringstream ss;
	ss << "[" << std::fixed << std::setprecision(3) << elapsed_sec << "s] "
		 << sender << " -> " << receiver << ": " << sanitize(text);
	std::string formatted = ss.str();

	if (options_.verbose) {
		std::cout << formatted << std::endl;
	}
	if (log_file_.is_open()) {
		log_file_ << formatted << "\n";
		log_file_.flush();
	}
}

void InteractiveTester::flush_line_buffer(std::string& buffer,
																					const std::string& sender,
																					const std::string& receiver,
																					double elapsed_sec,
																					bool force_flush) {
	size_t pos = 0;
	while ((pos = buffer.find('\n')) != std::string::npos) {
		std::string line = buffer.substr(0, pos);
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		log_message(sender, receiver, line, elapsed_sec);
		buffer.erase(0, pos + 1);
	}
	if (force_flush && !buffer.empty()) {
		std::string line = buffer;
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		log_message(sender, receiver, line, elapsed_sec);
		buffer.clear();
	}
}

int InteractiveTester::run() {
	if (!fs::exists(options_.solution_path)) {
		std::cerr << "Error: Solution binary does not exist: "
							<< options_.solution_path << std::endl;
		return 1;
	}
	if (!fs::exists(options_.checker_path)) {
		std::cerr << "Error: Checker binary does not exist: "
							<< options_.checker_path << std::endl;
		return 1;
	}

	Process sol_proc;
	Process chk_proc;

	if (!sol_proc.launch(options_.solution_path)) {
		std::cerr << "Error: Failed to launch solution binary: "
							<< options_.solution_path << std::endl;
		return 1;
	}

	if (!chk_proc.launch(options_.checker_path)) {
		std::cerr << "Error: Failed to launch checker binary: "
							<< options_.checker_path << std::endl;
		sol_proc.terminate();
		return 1;
	}

	auto start_time = std::chrono::steady_clock::now();

	std::string sol_line_buf;
	std::string chk_line_buf;

	char buffer[4096];
	bool sol_eof = false;
	bool chk_eof = false;
	bool timed_out = false;

	while (true) {
		auto now = std::chrono::steady_clock::now();
		double elapsed_sec =
				std::chrono::duration<double>(now - start_time).count();
		int elapsed_ms = static_cast<int>(elapsed_sec * 1000.0);

		if (elapsed_ms > options_.timeout_ms) {
			timed_out = true;
			break;
		}

		bool did_work = false;

		// Read Solution stdout -> write to Checker stdin
		if (!sol_eof) {
			int n = sol_proc.read_stdout(buffer, sizeof(buffer), sol_eof);
			if (n > 0) {
				did_work = true;
				chk_proc.write_stdin(buffer, static_cast<size_t>(n));
				sol_line_buf.append(buffer, static_cast<size_t>(n));
				flush_line_buffer(sol_line_buf, "SOL", "CHK", elapsed_sec, false);
			}
			if (sol_eof) {
				chk_proc.close_stdin();
			}
		}

		// Read Checker stdout -> write to Solution stdin
		if (!chk_eof) {
			int n = chk_proc.read_stdout(buffer, sizeof(buffer), chk_eof);
			if (n > 0) {
				did_work = true;
				sol_proc.write_stdin(buffer, static_cast<size_t>(n));
				chk_line_buf.append(buffer, static_cast<size_t>(n));
				flush_line_buffer(chk_line_buf, "CHK", "SOL", elapsed_sec, false);
			}
			if (chk_eof) {
				sol_proc.close_stdin();
			}
		}

		bool sol_running = sol_proc.is_running();
		bool chk_running = chk_proc.is_running();

		if (!sol_running && !chk_running && sol_eof && chk_eof) {
			break;
		}

		if (!sol_running && sol_eof) {
			chk_proc.close_stdin();
		}
		if (!chk_running && chk_eof) {
			sol_proc.close_stdin();
		}

		if (!sol_running && !chk_running) {
			// Drain any remaining output
			while (!sol_eof) {
				int n = sol_proc.read_stdout(buffer, sizeof(buffer), sol_eof);
				if (n > 0) {
					chk_proc.write_stdin(buffer, static_cast<size_t>(n));
					sol_line_buf.append(buffer, static_cast<size_t>(n));
					flush_line_buffer(sol_line_buf, "SOL", "CHK", elapsed_sec, false);
				}
			}
			while (!chk_eof) {
				int n = chk_proc.read_stdout(buffer, sizeof(buffer), chk_eof);
				if (n > 0) {
					sol_proc.write_stdin(buffer, static_cast<size_t>(n));
					chk_line_buf.append(buffer, static_cast<size_t>(n));
					flush_line_buffer(chk_line_buf, "CHK", "SOL", elapsed_sec, false);
				}
			}
			break;
		}

		if (!did_work) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	auto now = std::chrono::steady_clock::now();
	double final_elapsed =
			std::chrono::duration<double>(now - start_time).count();

	flush_line_buffer(sol_line_buf, "SOL", "CHK", final_elapsed, true);
	flush_line_buffer(chk_line_buf, "CHK", "SOL", final_elapsed, true);

	if (timed_out) {
		sol_proc.terminate();
		chk_proc.terminate();
		std::cout << "\n[RESULT] Time Limit Exceeded (" << options_.timeout_ms
							<< " ms limit reached)" << std::endl;
		return 124;
	}

	int sol_exit = sol_proc.wait(1000);
	int chk_exit = chk_proc.wait(1000);

	std::cout << "\n[RESULT] Finished in " << std::fixed << std::setprecision(3)
						<< final_elapsed << "s" << std::endl;
	std::cout << "Solution exit code: " << sol_exit << std::endl;
	std::cout << "Checker exit code:  " << chk_exit << std::endl;

	if (sol_exit != 0 || chk_exit != 0) {
		return 1;
	}

	return 0;
}
