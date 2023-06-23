#ifndef FT_YACC_POC_COMMANDLINE_HPP
#define FT_YACC_POC_COMMANDLINE_HPP
#include <string>

struct CommandLine {
	std::string input_file;
	std::string output_file;
	CommandLine(int argc, char **argv);
};


#endif
