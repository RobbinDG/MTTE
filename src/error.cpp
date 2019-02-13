#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>

#include "error.h"

void error::die(const std::string s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[1;1H", 6);
 	std::cout << s << strerror(1) << std::endl;
	exit(1);
}