#include "Diagnostic.hpp"
#include <stack>
#include <map>
#include <memory>
#include "CommandLine.hpp"
#include "scanner.yy.hpp"

extern std::shared_ptr<CommandLine>	commandLine;
extern std::shared_ptr<yyLexer>		lexer;

Diagnostic::Error::Error(std::string f, std::string fl, size_t l, size_t c, std::string d, Diagnostic::Error::Severity s) : file(f), full_line(fl), lineno(l), column(c), description(d), severity(s) {}

static std::string	get_full_line()
{
	std::string current_line = lexer->yyline_begin + lexer->text();
	std::stack<char> unput_stack;
	while (!current_line.ends_with('\n'))
	{
		char c = lexer->input();
		current_line.push_back(c);
		unput_stack.push(c);
	}
	for (;!unput_stack.empty();unput_stack.pop())
		lexer->unput(unput_stack.top());
	return current_line;
}

Diagnostic::Error::Error(std::string d, Error::Severity s) {
	file = commandLine->input_file;
	full_line =  get_full_line();
	lineno = lexer->yylineno;
	column = lexer->yycolno;
	description = d;
	severity = s;
}

int Diagnostic::Error::print() const {
	int margin = std::max(std::to_string(this->lineno).size() + 1, this->file.size());
	margin += 8 - (margin + 1) % 8;
	return printf("%*s:%zu:%zu: %s\n", margin,this->file.c_str(), this->lineno, this->column, this->description.c_str())
	+ printf("%*zu |%s", margin - 1, this->lineno, this->full_line.c_str())
	+ printf("%*c |%*c", margin - 1, ' ', this->column + 1, '^');
}



void Diagnostic::emit_error(std::string description, Error::Severity severity)
{
	Error e{
		commandLine->input_file,
		get_full_line(),
		lexer->yylineno,
		lexer->yycolno,
		description,
		severity
	};

	e.print();
}