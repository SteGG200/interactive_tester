#include <iostream>

int main() {
	int low = 1, high = 100;
	while (low <= high) {
		int mid = (low + high) / 2;
		std::cout << mid << std::endl;

		std::string res;
		if (!(std::cin >> res)) break;

		if (res == "=") {
			break;
		} else if (res == "<") {
			low = mid + 1;
		} else if (res == ">") {
			high = mid - 1;
		}
	}
	return 0;
}
