#include <iostream>

int main() {
	const int secret = 42;
	int guess;
	while (std::cin >> guess) {
		if (guess < secret) {
			std::cout << "<\n" << std::flush;
		} else if (guess > secret) {
			std::cout << ">\n" << std::flush;
		} else {
			std::cout << "=\n" << std::flush;
			return 0;
		}
	}
	return 1;
}
