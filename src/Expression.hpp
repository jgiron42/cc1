#ifndef CC1_POC_EXPRESSION_HPP
#define CC1_POC_EXPRESSION_HPP
#include "SymbolTable.hpp"
#include <functional>
#include "TAC.hpp"

namespace Expression {

	enum Operation {
		IDENTIFIER,
		CONSTANT,
		STRING_LITERAL,
		SIZEOF_TYPE,

		MEMBER_ACCESS,
		PTR_MEMBER_ACCESS,

		// unaries
		POST_INC,
		_UNARIES_START = POST_INC,
		POST_DEC,
		PRE_INC,
		PRE_DEC,
		ADDRESS, // special
		DEREFERENCE, // special
		UPLUS,
		UMINUS,
		BITWISE_NOT,
		LOGICAL_NOT,
		SIZEOF_EXPR, // special
		CAST, // special
		_UNARIES_END = CAST,

		// binaries
//		SUBSCRIPT, // special
		MUL,
		_BINARIES_START = MUL,
		DIV,
		MOD,
		ADD,
		SUB,
		BITSHIFT_LEFT,
		BITSHIFT_RIGHT,
		LESSER,
		GREATER,
		LESSER_EQUAL,
		GREATER_EQUAL,
		EQUAL,
		NOT_EQUAL,
		BITWISE_AND,
		BITWISE_XOR,
		BITWISE_OR,
		LOGICAL_AND,
		LOGICAL_OR,
		ASSIGNMENT,
		MUL_ASSIGNMENT,
		DIV_ASSIGNMENT,
		MOD_ASSIGNMENT,
		ADD_ASSIGNMENT,
		SUB_ASSIGNMENT,
		BITSHIFT_LEFT_ASSIGNMENT,
		BITSHIFT_RIGHT_ASSIGNMENT,
		BITWISE_AND_ASSIGNMENT,
		BITWISE_XOR_ASSIGNMENT,
		BITWISE_OR_ASSIGNMENT,
		COMMA,
		_BINARIES_END = COMMA,

		// ternaries
		TERNARY,

		// variadic
		CALL,
	};

	struct Node {
		Operation									operation;
		std::vector<Node>							operands;
		std::optional<Types::CType>					typeArg; // used only by cast and sizeof
		std::optional<const SymbolTable::Constant*>	constant;
		std::optional<std::string>					identifier;

		std::optional<Types::CType>	type; // result type of the expression
		TAC::Address						ret_address;
		std::vector<Types::CType>	operand_types; // types
	};

	bool	type_equivalence(const Types::CType &t1, const Types::CType &t2, bool ignore_const);
	bool	is_null(const Node &n);
	void	DeduceOneType(Expression::Node &n);
	void	DeduceType(Expression::Node &);
	void	EmitTac(Expression::Node &n);
	bool	is_convertible_to(const Types::CType &dst, const Types::CType &src);
	void	evaluate_constant_expression(Expression::Node &n);
}
#endif //CC1_POC_EXPRESSION_HPP
