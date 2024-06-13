#include "parser.tab.hpp"
#include "Ast.hpp"
#include "TAC.hpp"
#include "CodeGeneration.hpp"
#include "CommandLine.hpp"

Ast::TranslationUnit tree;
int yydebug;

std::shared_ptr<yyLexer>	lexer;
std::shared_ptr<yyParser>	parser;
std::shared_ptr<CommandLine>	commandLine;


int main(int argc, char **argv)
{
	std::istream *stream = &std::cin;
	std::ifstream file;
	commandLine.reset(new CommandLine(argc, argv));
	file.open(commandLine->input_file);
	stream = &file;
	if (!file.is_open())
	{
		std::cerr << "error: cant open file: " << commandLine->input_file << std::endl;
		return 1;
	}
	lexer.reset(new yyLexer(stream));

//	std::pair<int, YYSTYPE> ret;
//	while ((ret = lexer.yylex()).first)
//		std::cout << ret.first << std::endl
//	yydebug = 1;
	parser.reset(new yyParser(*lexer));
	if (parser->yyparse())
		return 1; // todo error management

	std::ofstream out(commandLine->output_file);
	if (!out.is_open())
	{
		std::cerr << "error: cant open file: " << commandLine->output_file << std::endl;
		return 1;
	}
	CodeGeneration::FileGenerator(out).generate();
}