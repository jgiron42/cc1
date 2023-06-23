#ifndef CC1_POC_CODEGENERATION_HPP
#define CC1_POC_CODEGENERATION_HPP
#include "TAC.hpp"
#include <queue>
#include <functional>
#include <iostream>
#define REGISTER_SIZE(R) (1 << (static_cast<int>(R) % 4))
#define BASE_REG(R) static_cast<Register>((static_cast<int>(R) & ~0b11))
#define GET_SUB_REG(R, SIZE) static_cast<Register>(static_cast<int>(BASE_REG(R)) + ((SIZE) >= 2) + ((SIZE) >= 4) + ((SIZE) >= 8))
#define CLEAN_REG(R) do {this->put_instruction("xor", GET_SUB_REG((R), 8), GET_SUB_REG((R), 8));} while(0)

namespace CodeGeneration
{
	enum class Register {
		AL,		AX,		EAX,	RAX,
		BL,		BX,		EBX,	RBX,
		CL,		CX,		ECX,	RCX,
		DL,		DX,		EDX,	RDX,
		BPL,	BP,		EBP,	RBP,
		SPL,	SP,		ESP,	RSP,
		SIL,	SI,		ESI,	RSI,
		DIL,	DI,		EDI,	RDI,
		R8B,	R8W,	R8D,	R8,
		R9B,	R9W,	R9D,	R9,
		R10B,	R10W,	R10D,	R10,
		R11B,	R11W,	R11D,	R11,
		R12B,	R12W,	R12D,	R12,
		R13B,	R13W,	R13D,	R13,
		R14B,	R14W,	R14D,	R14,
		R15B,	R15W,	R15D,	R15,
	};
	bool compare_reg(const Register &l, const Register &r);
	using enum Register;

	static const char *registers_strings[] = {
		"al",	"ax",	"eax",	"rax",
		"bl",	"bx",	"ebx",	"rbx",
		"cl",	"cx",	"ecx",	"rcx",
		"dl",	"dx",	"edx",	"rdx",
		"bpl",	"bp",	"ebp",	"rbp",
		"spl",	"sp",	"esp",	"rsp",
		"sil",	"si",	"esi",	"rsi",
		"dil",	"di",	"edi",	"rdi",
		"r8b",	"r8w",	"r8d",	"r8",
		"r9b",	"r9w",	"r9d",	"r9",
		"r10b",	"r10w",	"r10d",	"r10",
		"r11b",	"r11w",	"r11d",	"r11",
		"r12b",	"r12w",	"r12d",	"r12",
		"r13b",	"r13w",	"r13d",	"r13",
		"r14b",	"r14w",	"r14d",	"r14",
		"r15b",	"r15w",	"r15d",	"r15",
	};

	static const std::string operand_size_str[] = {
			"",
			"BYTE",
			"WORD",
			"",
			"DWORD",
			"",
			"",
			"",
			"QWORD",
	};

	static const Register param_registers[] = {
		RDI,
		RSI,
		RDX,
		RCX,
		R8,
		R9
	};
	struct Indirection;
	struct Operand {
		enum Type {
			NONE,
			REGISTER,
			INDIRECT,
			DIRECT,
			IMMEDIATE
		};
		Type		type;

		Register	reg;

		std::string	label;

		uint64_t	immediate_value;

		std::shared_ptr<Indirection> indirection;

		Operand() : type(NONE) {};
		// init register
		Operand(const Register &reg) : type(REGISTER), reg(reg) {};
		// init indirection
		explicit Operand(Indirection * const & i) : type(INDIRECT), indirection(i) {};
		// init direct
		Operand(const std::string &s) : type(DIRECT), label(s) {};
		// init immediate
		Operand(const uint64_t &i) : type(IMMEDIATE), immediate_value(i) {};
	};
	struct Indirection {
		TAC::Address	base;
		TAC::Address	index;
		size_t			scale;
		ssize_t			displacement;
	};

	class FileGenerator {
	private:
		std::ostream			&out;
		int						line;
		void					put(const std::string &);
		void					put_constant(const SymbolTable::Constant *);
	public:
		FileGenerator(std::ostream &);
		void			generate();
		int				get_lineno() const;
		friend class FunctionGenerator;
	};

	class FunctionGenerator {
	private:
		const SymbolTable::Function &function;
		FileGenerator				&gen;

		std::map<int, Register>							temp_to_register; // store the current register where is the temp
		std::map<TAC::Address, Operand>					map_variables; // store the current register where is the temp
		std::map<Register, const TAC::Address &, std::function<decltype(compare_reg)>>		allocated_registers; // todo rename
		std::set<Register, std::function<decltype(compare_reg)>>							available_registers;

		size_t								frame_size;

		std::vector<int>					instruction_to_asm; // store the position in the file of an instruction
		int									param_pos = 0;
		std::stack<Operand>					stack_args;
		void		init_registers();
		void		map_params();
		void		map(const TAC::Address &, const Operand &);
		Register	alloc_register(const std::optional<Register> & = std::nullopt);
		void 		spill(const Register&);
		void		free_register(Register);
		Operand		temp_to_operand(int);
		Operand		get_address_location(const TAC::Address &);
		Operand		address_to_operand(const TAC::Address &);
		void		free_temp(int);
		void		translate_instruction(const TAC::Instruction&);
		void		translate_bop(const TAC::Instruction &);
		void		add_param(const TAC::Instruction &);
		void		call(const TAC::Instruction &);
		void		move(const TAC::Address &dst, const TAC::Address &src);
		void		load(const Register &, const TAC::Address &);
		void		store(const TAC::Address &, const Register &);
		void		translate_jump(const TAC::Instruction &);
		std::string	operand_to_string(const Operand &a);
		std::string	indirection_to_string(const Indirection &);
		std::string	operand_to_string(const TAC::Address &a);
		void		put_instruction(const char *opcode, const Operand &a, const Operand &b);
		void		spill_params();
		void		enter();
		void		leave();
		void		put_name();
		static size_t		size_of(const Operand &o);
		size_t		size_of(const TAC::Address &o);

		auto	get_labels_pqueue() const;
		auto	get_last_usage_pqueue() const;

	public:
		FunctionGenerator(const SymbolTable::Function &, FileGenerator &);
		void	translate();
	};

	void	translate(const TAC::TacFunction &, std::ostream &);
}

#endif
