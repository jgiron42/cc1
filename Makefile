NAME = cc1

SRCS =	parser.tab.cpp \
		scanner.yy.cpp \
		main.cpp \
		SymbolTable.cpp \
		Expression.cpp \
		TAC.cpp \
		CodeGeneration.cpp \
		Diagnostic.cpp \
		CommandLine.cpp \
		Types.cpp \
		Ast.cpp

SRCS_DIR = src

OBJS_DIR = .obj

INCLUDE_DIR = srcs

CXXFLAGS = -g3 -std=c++20

LDFLAGS =

LEX=../ft_lex/ft_lex
YACC=../ft_yacc_poc/ft_yacc

include template_cpp.mk
