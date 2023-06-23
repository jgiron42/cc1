#include "parser.tab.hpp"
#include "Ast.hpp"
#include "TAC.hpp"
#include "CodeGeneration.hpp"
#include "CommandLine.hpp"

Ast::TranslationUnit tree;
int yydebug;

int main(int argc, char **argv)
{
	std::istream *stream = &std::cin;
	std::ifstream file;
	CommandLine commandLine(argc, argv);
	file.open(commandLine.input_file);
	stream = &file;
	if (!file.is_open())
	{
		std::cerr << "error: cant open file: " << commandLine.input_file << std::endl;
		return 1;
	}
	yyLexer lexer(stream);
//	std::pair<int, YYSTYPE> ret;
//	while ((ret = lexer.yylex()).first)
//		std::cout << ret.first << std::endl
//	yydebug = 1;
	yyParser parser(lexer);
	parser.yyparse();

	std::ofstream out(commandLine.output_file);
	if (!out.is_open())
	{
		std::cerr << "error: cant open file: " << commandLine.output_file << std::endl;
		return 1;
	}
	CodeGeneration::FileGenerator(out).generate();
}