


D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <stdio.h>
#include "parser.def.hpp"

int check_type(char *);
void count(char *yytext);
int yywrap();
int	parse_literal(const std::string &s);
void comment();
%}

%%
\/\*([^*]|\*[^/])*\*\/		count(yytext);
[[:blank:]]*#.*$		count(yytext);

"auto"			{ count(yytext); return(AUTO); }
"break"			{ count(yytext); return(BREAK); }
"case"			{ count(yytext); return(CASE); }
"char"			{ count(yytext); return(CHAR); }
"const"			{ count(yytext); return(CONST); }
"continue"		{ count(yytext); return(CONTINUE); }
"default"		{ count(yytext); return(DEFAULT); }
"do"			{ count(yytext); return(DO); }
"double"		{ count(yytext); return(DOUBLE); }
"else"			{ count(yytext); return(ELSE); }
"enum"			{ count(yytext); return(ENUM); }
"extern"		{ count(yytext); return(EXTERN); }
"float"			{ count(yytext); return(FLOAT); }
"for"			{ count(yytext); return(FOR); }
"goto"			{ count(yytext); return(GOTO); }
"if"			{ count(yytext); return(IF); }
"int"			{ count(yytext); return(INT); }
"long"			{ count(yytext); return(LONG); }
"register"		{ count(yytext); return(REGISTER); }
"return"		{ count(yytext); return(RETURN); }
"short"			{ count(yytext); return(SHORT); }
"signed"		{ count(yytext); return(SIGNED); }
"sizeof"		{ count(yytext); return(SIZEOF); }
"static"		{ count(yytext); return(STATIC); }
"struct"		{ count(yytext); return(STRUCT); }
"switch"		{ count(yytext); return(SWITCH); }
"typedef"		{ count(yytext); return(TYPEDEF); }
"union"			{ count(yytext); return(UNION); }
"unsigned"		{ count(yytext); return(UNSIGNED); }
"void"			{ count(yytext); return(VOID); }
"volatile"		{ count(yytext); return(VOLATILE); }
"while"			{ count(yytext); return(WHILE); }

{L}({L}|{D})*		{ count(yytext);return std::pair{check_type(yytext), std::string(yytext)};}

0[xX]{H}+{IS}?		{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stoull(yytext, 0, 16))}; }
0{D}+{IS}?		{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stoull(yytext, 0, 8))}; }
{D}+{IS}?		{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stoull(yytext))}; }
L?'(\\.|[^\\'])+'	{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(parse_literal(yytext))};}

{D}+{E}{FS}?		{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stod(yytext))}; }
{D}*"."{D}+({E})?{FS}?	{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stod(yytext))}; }
{D}+"."{D}*({E})?{FS}?	{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::stod(yytext))}; }

L?\"(\\.|[^\\"])*\"	{ count(yytext); return std::pair{CONSTANT, symbolTable.new_constant(std::string(yytext))};}

"..."			{ count(yytext); return(ELLIPSIS); }
">>="			{ count(yytext); return(RIGHT_ASSIGN); }
"<<="			{ count(yytext); return(LEFT_ASSIGN); }
"+="			{ count(yytext); return(ADD_ASSIGN); }
"-="			{ count(yytext); return(SUB_ASSIGN); }
"*="			{ count(yytext); return(MUL_ASSIGN); }
"/="			{ count(yytext); return(DIV_ASSIGN); }
"%="			{ count(yytext); return(MOD_ASSIGN); }
"&="			{ count(yytext); return(AND_ASSIGN); }
"^="			{ count(yytext); return(XOR_ASSIGN); }
"|="			{ count(yytext); return(OR_ASSIGN); }
">>"			{ count(yytext); return(RIGHT_OP); }
"<<"			{ count(yytext); return(LEFT_OP); }
"++"			{ count(yytext); return(INC_OP); }
"--"			{ count(yytext); return(DEC_OP); }
"->"			{ count(yytext); return(PTR_OP); }
"&&"			{ count(yytext); return(AND_OP); }
"||"			{ count(yytext); return(OR_OP); }
"<="			{ count(yytext); return(LE_OP); }
">="			{ count(yytext); return(GE_OP); }
"=="			{ count(yytext); return(EQ_OP); }
"!="			{ count(yytext); return(NE_OP); }
";"				{ count(yytext); return(';'); }
("{"|"<%")		{ count(yytext); return('{'); }
("}"|"%>")		{ count(yytext); return('}'); }
","				{ count(yytext); return(','); }
":"				{ count(yytext); return(':'); }
"="				{ count(yytext); return('='); }
"("				{ count(yytext); return('('); }
")"				{ count(yytext); return(')'); }
("["|"<:")		{ count(yytext); return('['); }
("]"|":>")		{ count(yytext); return(']'); }
"."				{ count(yytext); return('.'); }
"&"				{ count(yytext); return('&'); }
"!"				{ count(yytext); return('!'); }
"~"				{ count(yytext); return('~'); }
"-"				{ count(yytext); return('-'); }
"+"				{ count(yytext); return('+'); }
"*"				{ count(yytext); return('*'); }
"/"				{ count(yytext); return('/'); }
"%"				{ count(yytext); return('%'); }
"<"				{ count(yytext); return('<'); }
">"				{ count(yytext); return('>'); }
"^"				{ count(yytext); return('^'); }
"|"				{ count(yytext); return('|'); }
"?"				{ count(yytext); return('?'); }

[ \t\v\n\f]		{ count(yytext); }
.			{ printf("bad char\n"); /* ignore bad characters */ }

%%

int yywrap()
{
	return(1);
}


int column = 0;

void count(char *yytext)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
	ECHO;
}


int check_type(char *s)
{
	if (symbolTable.is_typename(s))
		return TYPE_NAME;
	return IDENTIFIER;
}

int	parse_literal(const std::string &s)
{
	if (s[0] != '\'')
		return -1;
	if (s[1] != '\\')
		return s[1];
	if (s[2] == 'x')
	{
		int n = 0;
		for (int i = 3;s[i]; i++)
		{
			n *= 16;
			if (isdigit(s[i]))
				n += s[i] - '0';
			else if (islower(s[i]))
				n += s[i] - 'a';
			else if (isupper(s[i]))
				n += s[i] - 'A';
		}
		return n;
	}
	else if (s[2] >= '0' && s[2] <= '8')
	{
		int n = 0;
		for (int i = 2;s[i]; i++)
			n = n * 8 + s[i] - '0';
		return n;
	}
	else switch (s[2])
		{
			case '\\':
				return '\\';
			case 'a':
				return '\a';
			case 'b':
				return '\b';
			case 'f':
				return '\f';
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case 'r':
				return '\r';
			case 'v':
				return '\v';
			default:
				return s[1];
		}
}