#ifndef INTERACTIVE_TESTER_HPP
#define INTERACTIVE_TESTER_HPP

#include <chrono>
#include <fstream>
#include <string>

struct TesterOptions {
	std::string solution_path;
	std::string checker_path;
	int timeout_ms = 5000;
	bool verbose = false;
	std::string log_path;
};

class InteractiveTester {
 public:
	explicit InteractiveTester(const TesterOptions& options);
	~InteractiveTester();

	// Run the interactive testing session.
	// Returns exit code (0 for success/clean exit, non-zero for
	// error/TLE/mismatch).
	int run();

 private:
	void log_message(const std::string& sender, const std::string& receiver,
									 const std::string& text, double elapsed_sec);
	void flush_line_buffer(std::string& buffer, const std::string& sender,
												 const std::string& receiver, double elapsed_sec,
												 bool force_flush = false);
	static std::string sanitize(const std::string& input);

	TesterOptions options_;
	std::ofstream log_file_;
};

#endif	// INTERACTIVE_TESTER_HPP
