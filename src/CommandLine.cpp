#include "CommandLine.hpp"
#include <unistd.h>
#include <iostream>

CommandLine::CommandLine(int argc, char **argv) :
	input_file(""),
	output_file("out.s")
{
	while (1)
		switch (getopt(argc, argv, "-o:"))
		{
			case 'o':
				this->output_file = optarg;
				break;
			case 1:
				this->input_file = optarg;
				break;
			case '?':
				exit(1);
			case -1:
				if (this->input_file.empty())
				{
					std::cerr << "Missing operand" << std::endl;
					exit(1);
				}
				return;
		}
}