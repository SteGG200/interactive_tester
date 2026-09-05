#include <iostream>
#include <string>

#include "argparse.hpp"
#include "interactive_tester.hpp"

int main(int argc, char* argv[]) {
	argparse::ArgumentParser program("interactive_tester", "1.0.0");

	program.add_argument("solution").help("Path to contestant's solution binary");

	program.add_argument("checker").help("Path to checker binary");

	program.add_argument("-t", "--timeout")
			.help("Time limit in milliseconds")
			.default_value(5000)
			.scan<'i', int>();

	program.add_argument("-v", "--verbose")
			.help("Print communication between processes to stdout")
			.default_value(false)
			.implicit_value(true);

	program.add_argument("-l", "--log")
			.help("Path to log file for recording communication")
			.default_value(std::string(""));

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	TesterOptions options;
	options.solution_path = program.get<std::string>("solution");
	options.checker_path = program.get<std::string>("checker");
	options.timeout_ms = program.get<int>("--timeout");
	options.verbose = program.get<bool>("--verbose");
	options.log_path = program.get<std::string>("--log");

	InteractiveTester tester(options);
	return tester.run();
}
