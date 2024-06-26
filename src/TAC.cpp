#include "TAC.hpp"
#include <iostream>

namespace TAC
{
	TacFunction					*currentFunction;
	std::deque<TacFunction>	functions;

	TacFunction::TacFunction(SymbolTable::Function &s) {s.tac = this;}

	int TacFunction::new_temp(const Types::CType &type) {
		temps.emplace_back(type);
		last_usage.emplace_back();
		return temps.size() - 1;
	}

	void TacFunction::set_temp_type(int id, const Types::CType &type) {
		this->temps[id] = type;
	}

	void TacFunction::add_instruction(const TAC::Instruction &i) {
		if (holds_alternative<int>(i.ret))
			this->last_usage[get<int>(i.ret)] = this->instructions.size();
		if (holds_alternative<int>(i.oper1))
			this->last_usage[get<int>(i.oper1)] = this->instructions.size();
		if (holds_alternative<int>(i.oper2))
			this->last_usage[get<int>(i.oper2)] = this->instructions.size();
		this->instructions.emplace_back(i);
	}

	std::ostream &operator<<(std::ostream &ostream, const Address &addr)
	{
		switch (addr.index())
		{
			case 1:
				return ostream << "t" << get<1>(addr);
			case 2:
				return ostream << get<2>(addr)->name;
			case 3:
			{
				auto val = get<3>(addr)->value;
				switch (val.index())
				{
					case 0:
						return ostream << get<0>(val);
					case 1:
						return ostream << get<1>(val);
					default:
					case 2:
						return ostream << get<2>(val);
				}
			}
			case 4:
				return ostream << "l" << int(get<4>(addr));
			case 5:
				return ostream << get<5>(addr);
			default:
				throw std::runtime_error("shouldn't be reached");

		}
	}

	const std::vector<Instruction> &TacFunction::get_instructions() const {
		return this->instructions;
	}

	const std::vector<int> &TacFunction::get_last_usages() const {
		return this->last_usage;
	}

	bool Instruction::is_bop() const {
		return this->op >= ADD && this->op < ASSIGN;
	}

	bool Instruction::is_oop() const {
		return this->op >= NEG && this->op <= LOGICAL_NOT;
	}

	bool Instruction::is_jump() const {
		return this->op >= JUMP && this->op <= JUMP_GREATER_EQUAL;
	}

	void Instruction::print() const {
		static const char *instructions_strings[] = {
				"NO INSTRUCTION",
				"ADD",
				"SUB",
				"MOD",
				"MUL",
				"DIV",
				"SHIFT_LEFT",
				"SHIFT_RIGHT",
				"LESSER",
				"GREATER",
				"LESSER_EQUAL",
				"GREATER_EQUAL",
				"EQUAL",
				"NOT_EQUAL",
				"BITWISE_AND",
				"BITWISE_XOR",
				"BITWISE_OR",
				"LOGICAL_AND",
				"LOGICAL_OR",
				"ASSIGN",

				"RETURN",
				"JUMP",
				"==",
				"!=",
				"<",
				">",
				"<=",
				">=",

				"PARAM",
				"CALL",
		};
		if (this->op == TAC::JUMP)
			std::cout << "jump to " << this->ret;
		else if (this->is_jump())
		{
			std::cout << "if (";
			if (this->oper1.index())
				std::cout << this->oper1 << " ";
			std::cout << instructions_strings[this->op];
			if (this->oper2.index())
				std::cout << " " << this->oper2;
			std::cout << ") jump to " << this->ret;
		}
		else
		{
			if (this->ret.index())
				std::cout << this->ret << " = ";
			std::cout << instructions_strings[this->op];
			if (this->oper1.index())
				std::cout << " " << this->oper1;
			if (this->oper2.index())
				std::cout << " " << this->oper2;
		}
		std::cout << std::endl;

	}

	void TacFunction::print() const {
		for (size_t i = 0; i < instructions.size(); i++) {
			for (size_t j = 0; j < this->labels.size(); j++) // ugly but it is just for debugging
				if (this->labels[j] == i)
					std::cout << "l" << j << ": ";
			instructions[i].print();
		}
	}

	Label TacFunction::new_label(bool here) {
		this->labels.emplace_back(-1);
		if (here)
			this->set_label(this->labels.size() - 1);
		return this->labels.size() - 1;
	}

	void TacFunction::set_label(const TAC::Label &l) {
		this->labels[l] = this->instructions.size();
	}

}