#ifndef CC1_POC_TAC_HPP
#define CC1_POC_TAC_HPP
#include <iostream>
#include <vector>
#include "SymbolTable.hpp"

namespace CodeGeneration {
	class FunctionGenerator;
}

namespace TAC {

	enum Operation {
		ADD = 1,
		SUB,
		MOD,
		MUL,
		DIV,
		SHIFT_LEFT,
		SHIFT_RIGHT,

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

		ASSIGN,

		NEG,

		BITWISE_NOT,
		LOGICAL_NOT,

		DEREFERENCE,
		ADDRESS,

		JUMP,
		JUMP_EQUAL,
		JUMP_NOT_EQUAL,
		JUMP_LESSER,
		JUMP_GREATER,
		JUMP_LESSER_EQUAL,
		JUMP_GREATER_EQUAL,

		RETURN,

		PARAM,
		CALL,
	};

	class Label {
	private:
		int pos;
	public:
		Label() : pos(-1) {};
		Label(int p) : pos(p) {}
		operator const int() const {return this->pos;}
	};

	typedef std::variant<
		std::monostate,
		int,
		SymbolTable::Ordinary*,
		const SymbolTable::Constant*,
		Label,
		std::string
	> Address;
	struct Instruction {
		Address					ret;
		Operation				op;
		Address					oper1;
		Address					oper2;

		size_t					operation_size;
		bool					operation_sign;

		bool is_bop() const;
		bool is_oop() const;
		bool is_jump() const;
		void print() const;
	};

	class TacFunction {
	private:
		std::vector<Instruction>		instructions;
		std::vector<Types::CType>		temps;
		std::vector<int>				last_usage;
		std::vector<int>				labels;
	public:
		TacFunction(SymbolTable::Function &);
		int					new_temp(const Types::CType &type);
		void				set_temp_type(int id, const Types::CType &type);
		Label				new_label(bool here = false);
		void				set_label(const Label &); // set the label position at the current position in the program
		void				add_instruction(const Instruction &i);

		const std::vector<Instruction>	&get_instructions() const;
		const std::vector<int>			&get_last_usages() const;

		void	print_address(const Address&);
		void	print() const;
		friend class CodeGeneration::FunctionGenerator;
	};

	extern std::deque<TacFunction>	functions;
	extern TacFunction				*currentFunction;
}


#endif
