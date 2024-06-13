#include "CodeGeneration.hpp"

namespace CodeGeneration
{
	bool compare_reg(const Register &l, const Register &r)
	{
		return BASE_REG(l) < BASE_REG(r);
	}

	FunctionGenerator::FunctionGenerator(const SymbolTable::Function &f, FileGenerator &fg) : function(f), gen(fg), frame_size(0), allocated_registers(&compare_reg), available_registers(&compare_reg) {}
	using enum Register;

	std::string FunctionGenerator::indirection_to_string(const Indirection &i) {
		if	((i.base.type != Operand::REGISTER && i.base.type != Operand::DIRECT) ||
			(i.index.type != Operand::IMMEDIATE && i.index.type != Operand::REGISTER))
			throw std::runtime_error("shouldn't be reached");
		return "[" + operand_to_string(i.base, true) + "+" + operand_to_string(i.index, true) + "*" + std::to_string(i.scale) + "+" + std::to_string(i.displacement) + "]";
	}

	std::string FunctionGenerator::operand_to_string(const Operand &a, bool is_sub_operand = false) { // todo: bof bof
		switch (a.type)
		{
			case Operand::REGISTER:
				return registers_strings[static_cast<int>(a.reg)];
			case Operand::INDIRECT: {
				if ((a.indirection->base.type != Operand::REGISTER && a.indirection->base.type != Operand::DIRECT) ||
					(a.indirection->index.type != Operand::IMMEDIATE &&
							a.indirection->index.type != Operand::REGISTER))
					throw std::runtime_error("shouldn't be reached");
				return operand_size_str[a.indirection->scale] + " PTR " + indirection_to_string(*a.indirection);
			}
			case Operand::DIRECT:
				if (is_sub_operand)
					return a.label;
				else
					return "OFFSET " + a.label;
			case Operand::IMMEDIATE:
				return std::to_string(a.immediate_value);
			default:
				throw std::runtime_error("shouldn't be reached");
		}
	}

	void FunctionGenerator::put_instruction(const char *opcode, const Operand &a = {}, const Operand &b = {})
	{
		std::string line;
		line += std::string(opcode) + " ";
		if (a.type)
			line += operand_to_string(a);
		if (b.type)
			line += ", " + operand_to_string(b);
		this->gen.put(line);
	}

	void FunctionGenerator::init_registers() {
		const auto registers = {
				EDI,
				ESI,
				ECX,
				EDX,
		};
//		this->available_registers = decltype(this->available_registers)(registers.begin(), registers.end(), &compare_reg);
	}

	void FunctionGenerator::spill(const CodeGeneration::Register &r) {
//		printf("spill\n");
		auto it = this->allocated_registers.find(r);
		if (it == this->allocated_registers.end())
			return;
		this->put_instruction("push", r);
		this->frame_size += 8;
		this->map_variables[it->second] = Operand(new Indirection{GET_SUB_REG(RBP, REG_SIZE), 0, 8, -(ssize_t)this->frame_size});
		this->allocated_registers.erase(r);
	}

	Register FunctionGenerator::alloc_register(const std::optional<Register> &reg) {
		Register ret;
		if (reg)
		{
			if (this->allocated_registers.contains(reg.value()))
				this->spill(reg.value());
			ret = reg.value();
		}
		else {
			if (available_registers.empty())
				throw std::runtime_error("no registers available");
			ret = *available_registers.begin();
		}
		available_registers.erase(ret);
		return ret;
	}

	void FunctionGenerator::free_register(Register r) {
		available_registers.insert(r);
	}

	Operand FunctionGenerator::alloc_temporary(size_t size) {
		Operand mapping;
		try { // map on register
			Register r = GET_SUB_REG(alloc_register(), size);
//			printf("size: %zu %s\n", size, registers_strings[int(r)]);
			mapping.type = Operand::REGISTER;
			mapping.reg = r;
		}
		catch (std::exception&) // no more registers, map on stack
		{
//			this->put_instruction("push", 0); todo
			mapping.type = Operand::INDIRECT;
			this->frame_size += size;
			auto *i = new Indirection {GET_SUB_REG(RBP, REG_SIZE), 0, size, -(ssize_t)this->frame_size};
//			this->frame_size += PTR_SIZE;
			mapping.indirection = std::shared_ptr<Indirection>(i);
		}
		return mapping;
	}

	Operand FunctionGenerator::temp_to_operand(int temp) {
		if (holds_alternative<Types::FunctionType>(this->function.tac->temps[temp].type))
			return alloc_temporary(PTR_SIZE); // todo
		else
		{
			auto ret = alloc_temporary(symbolTable.size_of(this->function.tac->temps[temp]));
			ret.is_signed = Types::is_signed(this->function.tac->temps[temp]);
			return ret;
		}
	}

	Operand FunctionGenerator::address_to_operand(const TAC::Address &addr) {
		Operand ret;
		switch (addr.index())
		{
			default:
			case 0:
				throw std::runtime_error("code generator: invalid operand");
			case 1:
				ret = temp_to_operand(get<int>(addr));
				break;
			case 2:
			{
				auto &o = get<SymbolTable::Ordinary *>(addr);
				if (o->storage == SymbolTable::Ordinary::AUTO)
					ret = Operand(new Indirection{GET_SUB_REG(RBP, REG_SIZE), 0, symbolTable.size_of(o->type), -o->offset},
								  Types::is_signed(o->type));
				else
					ret = Operand(new Indirection{o->name, 0, symbolTable.size_of(o->type), 0}, Types::is_signed(o->type));
			}
				break;
			case 3:
			{
				const auto &constant = get<const SymbolTable::Constant *>(addr);
				switch (constant->value.index()) {
					case 0:
						ret = {".LC" + std::to_string(constant->id), Types::is_signed(constant->type)};
				break;
					case 1:
						ret = {get<uintmax_t>(constant->value), Types::is_signed(constant->type)};
				break;
					case 2:
						ret = {".LC" + std::to_string(constant->id), Types::is_signed(constant->type)};
					break;
				}
				break;
			}
			case 4:
				if (!map_labels.contains(get<4>(addr)))
					map_labels[get<4>(addr)] = gen.get_label();
				ret = {".L" + std::to_string(map_labels[get<4>(addr)])};
				break;
			case 5:
				Types::CType type = symbolTable.retrieve_ordinary(get<5>(addr))->type;
//				symbolTable.sym
				ret = Operand(new Indirection{get<5>(addr), 0, symbolTable.size_of(type), 0}, Types::is_signed(type));
//				ret = {"OFFSET " + get<5>(addr)};
				break;
		}
		if (ret.type == Operand::REGISTER)
			this->allocated_registers.insert({ret.reg, addr});
		return ret;
	}

	std::map<TAC::Address, Operand>::iterator	FunctionGenerator::add_mapping(const TAC::Address &addr, Operand &&op)
	{
		return this->map_variables.insert({addr, op}).first;
	}

	Operand FunctionGenerator::get_address_location(const TAC::Address &addr) {
		if (auto it = this->map_variables.find(addr); it != this->map_variables.end())
			return it->second;
		return add_mapping(addr, address_to_operand(addr))->second;
	}

	void FunctionGenerator::free_temp(int temp) {
		if (auto it = this->map_variables.find(temp); it != this->map_variables.end())
		{
			if (it->second.type == Operand::REGISTER)
				this->free_register(it->second.reg);
			else
				;//todo
			this->map_variables.erase(temp);
		}
	}

	void FunctionGenerator::translate_bop(const TAC::Instruction &i) {
		Operand left(GET_SUB_REG(EAX, i.operation_size), i.operation_sign);
		Operand right(GET_SUB_REG(EBX, i.operation_size), i.operation_sign);
		Operand ret = left;

		load(left, i.oper1);
		load(right, i.oper2);

		switch (i.op)
		{
			case TAC::ADD:
				put_instruction("add", left, right);
				break;
			case TAC::SUB:
				put_instruction("sub", left, right);
				break;
			case TAC::MUL:
//				put_instruction("mov", GET_SUB_REG(EAX, i.operation_size), left); // todo ???
				if (i.operation_sign)
					put_instruction("imul", right);
				else
					put_instruction("mul", right);
//				this->move(ret, GET_SUB_REG(EAX, size_of(i.ret)))
				break;
			case TAC::DIV:
				put_instruction("xor", GET_SUB_REG(EDX, size_of(i.ret)), GET_SUB_REG(EDX, size_of(i.ret)));
//				put_instruction("mov", GET_SUB_REG(EAX, size_of(i.ret)), left); // todo ???
				if (i.operation_sign)
					put_instruction("idiv", right);
				else
					put_instruction("div", right);
//				put_instruction("mov", left, GET_SUB_REG(EAX, size_of(i.ret)));
				break;
			case TAC::MOD:
				put_instruction("xor", GET_SUB_REG(EDX, size_of(i.ret)), GET_SUB_REG(EDX, size_of(i.ret)));
//				put_instruction("mov", GET_SUB_REG(EAX, size_of(i.ret)), left);
				if (i.operation_sign)
					put_instruction("idiv", right);
				else
					put_instruction("div", right);
//				put_instruction("mov", left, GET_SUB_REG(EDX, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(EDX, size_of(i.ret)));
				break;
			case TAC::SHIFT_LEFT:
				put_instruction("shl", left, right);
				break;
			case TAC::SHIFT_RIGHT:
				put_instruction("shr", left, right);
				break;
			case TAC::BITWISE_AND:
				put_instruction("and", left, right);
				break;
			case TAC::BITWISE_XOR:
				put_instruction("xor", left, right);
				break;
			case TAC::BITWISE_OR:
				put_instruction("or", left, right);
				break;
			case TAC::LESSER:
				put_instruction("cmp", left, right);
				if (i.operation_sign)
					put_instruction("setl", GET_SUB_REG(left.reg, 1));
				else
					put_instruction("setb", GET_SUB_REG(left.reg, 1));
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
				break;
			case TAC::LESSER_EQUAL:
				put_instruction("cmp", left, right);
				if (i.operation_sign)
					put_instruction("setle", GET_SUB_REG(left.reg, 1));
				else
					put_instruction("setbe", GET_SUB_REG(left.reg, 1));
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
				break;
			case TAC::GREATER:
				put_instruction("cmp", left, right);
				if (i.operation_sign)
					put_instruction("setg", GET_SUB_REG(left.reg, 1));
				else
					put_instruction("seta", GET_SUB_REG(left.reg, 1));
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
				break;
			case TAC::GREATER_EQUAL:
				put_instruction("cmp", left, right);
				if (i.operation_sign)
					put_instruction("setge", GET_SUB_REG(left.reg, 1));
				else
					put_instruction("setae", GET_SUB_REG(left.reg, 1));
//				this->move(left, GET_SUB_REG(left, 1)) todo ?
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
				break;
			case TAC::EQUAL:
				put_instruction("cmp", left, right);
				put_instruction("sete", GET_SUB_REG(left.reg, 1));
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
				break;
			case TAC::NOT_EQUAL:
				put_instruction("cmp", left, right);
				put_instruction("setne", GET_SUB_REG(left.reg, 1));
				ret = Operand(GET_SUB_REG(left.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(left.reg, 1));
//				this->move(left, GET_SUB_REG(left.reg, 1));
				break;
			default:
				std::cerr << "unknown tac instruction!" << std::endl;
		}
		store(i.ret, ret);
	}


	void FunctionGenerator::translate_oop(const TAC::Instruction &i) {
		Operand reg(GET_SUB_REG(EAX, i.operation_size), i.operation_sign);
		Operand ret = reg;

		load(reg, i.oper1);
		switch (i.op)
		{
			case TAC::LOGICAL_NOT:
				put_instruction("cmp", reg, 0);
				put_instruction("sete", GET_SUB_REG(reg.reg, 1));
				ret = Operand(GET_SUB_REG(reg.reg, size_of(i.ret)));
				this->move(ret, GET_SUB_REG(reg.reg, 1));
				break;
			case TAC::BITWISE_NOT:
				put_instruction("not", reg);
				break;
			case TAC::NEG:
				put_instruction("neg", reg);
				break;
			default:
				std::cerr << "unknown tac instruction!" << std::endl;
		}
		store(i.ret, ret);
	}


	void FunctionGenerator::translate_logical_operator(const TAC::Instruction &i) {
		switch (i.op)
		{
			case TAC::LOGICAL_AND: // todo
			{
				put_instruction("cmp", get_address_location(i.oper1), 0);
				put_instruction("setne", AL);
				put_instruction("cmp", get_address_location(i.oper2), 0);
				put_instruction("setne", BL);
				put_instruction("and", AL, BL);
				this->move(GET_SUB_REG(RAX, REG_SIZE), AL);
				this->store(i.ret, GET_SUB_REG(RAX, REG_SIZE));
				break;
			}
			case TAC::LOGICAL_OR: // todo
			{
				put_instruction("cmp", get_address_location(i.oper1), 0);
				put_instruction("setne", AL);
				put_instruction("cmp", get_address_location(i.oper2), 0);
				put_instruction("setne", BL);
				put_instruction("or", AL, BL);
				this->move(GET_SUB_REG(RAX, REG_SIZE), AL);
				this->store(i.ret, GET_SUB_REG(RAX, REG_SIZE));
				break;
			}
			case TAC::LOGICAL_NOT:
			{
				put_instruction("cmp", get_address_location(i.oper1), 0);
				put_instruction("sete", AL);
				this->move(GET_SUB_REG(RAX, REG_SIZE), AL);
				this->store(i.ret, GET_SUB_REG(RAX, REG_SIZE));
				break;
			}
			default:
				std::cerr << "unknown tac instruction!" << std::endl;
		}
	}

	void FunctionGenerator::translate_jump(const TAC::Instruction &i) {
		if (this->function.tac->labels[get<TAC::Label>(i.ret)] < 0)
			throw std::runtime_error("no dst for label l" + std::to_string(get<TAC::Label>(i.ret)));

		if (i.op != TAC::JUMP) {
			Operand left(GET_SUB_REG(EAX, i.operation_size), i.operation_sign);
			Operand right(GET_SUB_REG(EBX, i.operation_size), i.operation_sign);
			load(left, i.oper1);
			load(right, i.oper2);
			this->put_instruction("cmp", left, right);
		}
		std::string opcodes[] = { // todo sign
				"jmp",
				"je",
				"jne",
				"jg",
				"jge",
				"jl",
				"jle"
		};
		this->put_instruction(opcodes[i.op - TAC::JUMP].c_str(), get_address_location(i.ret));
	}

	void FunctionGenerator::add_param(const TAC::Instruction &i) {
		this->stack_args.push(i);
	}

	size_t	FunctionGenerator::size_of(const Operand &o)
	{
		switch (o.type)
		{
			case Operand::REGISTER:
				return REGISTER_SIZE(o.reg);
			case Operand::INDIRECT:
				return o.indirection->scale;
			case Operand::DIRECT:
				return REG_SIZE; // todo
			case Operand::IMMEDIATE:
				return REG_SIZE; // todo
			default:
				throw std::runtime_error("shouldn't be reached");
		}
	}

	size_t	FunctionGenerator::size_of(const TAC::Address &o)
	{
		return this->size_of(get_address_location(o));
	}

	void FunctionGenerator::call(const TAC::Instruction &i) {
//		auto addr = get_address_location(i.oper1);
//		if ()`
//		assert(addr.type == Operand::INDIRECT);
//		addr = addr.indirection->base; // todo?
		this->load(GET_SUB_REG(RAX, REG_SIZE), i.oper1);

		size_t padding = (16 - (this->frame_size % 16)) % 16;
//		if (padding)
//			this->put_instruction("sub", GET_SUB_REG(RSP, REG_SIZE), padding); // todo
		this->put_instruction("sub", GET_SUB_REG(RSP, REG_SIZE), frame_size + padding);

		for (;!this->stack_args.empty(); this->stack_args.pop())
		{
			this->load({GET_SUB_REG(RBX, this->stack_args.top().operation_size), this->stack_args.top().operation_sign}, this->stack_args.top().oper1);
			this->put_instruction("push", GET_SUB_REG(RBX, REG_SIZE));
		}
		this->put_instruction("call", GET_SUB_REG(RAX, REG_SIZE));

//		if (padding)
//			this->put_instruction("add", GET_SUB_REG(RSP, REG_SIZE), padding);
		this->put_instruction("add", GET_SUB_REG(RSP, REG_SIZE), frame_size + padding);

//		for (;param_pos > 0; param_pos--)
//			this->put_instruction("pop", param_registers[param_pos - 1]); //todo: reload spilled
		this->param_pos = 0;
		if (!holds_alternative<std::monostate>(i.ret))
			this->store(i.ret, GET_SUB_REG(RAX, REG_SIZE));
	}

	// load an address in RAX or RBX
	void FunctionGenerator::load(const Operand &reg, Operand location) {
		assert(reg.type == Operand::REGISTER); // todo

		if (location.type == Operand::INDIRECT)
		{

//			auto l_base = get_address_location(location.indirection->base);
//			auto l_index = get_address_location(location.indirection->index);
			if (location.indirection->base.type == Operand::INDIRECT || location.indirection->index.type == Operand::INDIRECT)
			{
				this->load(GET_SUB_REG(reg.reg, REG_SIZE), location.indirection->base);
//				put_instruction("mov", GET_SUB_REG(reg, REG_SIZE), location.indirection->base); // todo:  32bit
//				this->map(location.indirection->base, GET_SUB_REG(reg, REG_SIZE));

				location.indirection.reset(new Indirection(*location.indirection)); // todo: AAAAAAAAAAAAAAAAHHHHHHH!!!!!!!!!
				location.indirection->base = GET_SUB_REG(reg.reg, REG_SIZE);
//				l_base = get_address_location(location.indirection->base);
//				l_index = get_address_location(location.indirection->index);
				// todo: redo
			}
			move(reg, location);
//			put_instruction("mov", GET_SUB_REG(reg, this->size_of(location)), location);
		}
		else
			move(reg, location);
//			put_instruction("mov", GET_SUB_REG(reg, this->size_of(location)), location);
	}

	void FunctionGenerator::load(const Operand &reg, const TAC::Address &addr) {
		this->load(reg, get_address_location(addr));
	}

	void FunctionGenerator::store(const TAC::Address &address, const CodeGeneration::Operand &reg) {
		assert(reg.type == Operand::REGISTER); // todo
		const auto &location = get_address_location(address);
		if (location.type == Operand::INDIRECT)
		{
//			const auto &l_base = get_address_location(location.indirection->base);
//			const auto &l_index = get_address_location(location.indirection->index);
			if (location.indirection->base.type == Operand::INDIRECT || location.indirection->index.type == Operand::INDIRECT)
			{
				this->load(GET_SUB_REG(RBX, PTR_SIZE), location.indirection->base); // todo: RBX?
				location.indirection->base = GET_SUB_REG(RBX, PTR_SIZE);

			}
			move(location, reg);
//			put_instruction("mov", location, GET_SUB_REG(reg, this->size_of(location)));
		}
		else
			move(location, reg);
//			put_instruction("mov", location, GET_SUB_REG(reg, this->size_of(location)));
	}

//	void FunctionGenerator::move(const TAC::Address &dst, const TAC::Address &src) {
	void FunctionGenerator::move(Operand l1, Operand l2) {
		// todo check for address - address mov
//		auto l1 = get_address_location(dst);
//		auto l2 = get_address_location(src);
		std::string opcode = "mov";

		if (size_of(l1) > size_of(l2))
		{
			if (l2.is_signed)
				opcode = "movsx"; // todo ?
			else
				opcode = "movzx";
		}
		else if (size_of(l1) < size_of(l2))
		{
			switch (l2.type)
			{
				case Operand::NONE:
					assert(false);
				case Operand::REGISTER:
					l2 = Operand(GET_SUB_REG(l2.reg, size_of(l1)), l2.is_signed);
					break;
				case Operand::INDIRECT:
					l2 = Operand(new Indirection(l2.indirection->base, l2.indirection->index, size_of(l1), l2.indirection->displacement), l2.is_signed);
					break;
				case Operand::DIRECT:
					assert(false);
				case Operand::IMMEDIATE:
					break;
			}
		}
		// todo else?

		put_instruction(opcode.c_str(), l1, l2);
	}

	void FunctionGenerator::translate_instruction(const TAC::Instruction& i) {
		this->instruction_to_asm.emplace_back(this->gen.get_lineno());

//		i.print();

		if (i.op == TAC::LOGICAL_AND || i.op == TAC::LOGICAL_OR || i.op == TAC::LOGICAL_NOT)
			this->translate_logical_operator(i);
		else if (i.is_bop())
			this->translate_bop(i);
		else if (i.is_oop())
			this->translate_oop(i);
		else if (i.is_jump())
			this->translate_jump(i);
		else if (i.op == TAC::ASSIGN)
		{
			Operand reg = {GET_SUB_REG(RAX, i.operation_size), i.operation_sign};
			this->load(reg, i.oper1);
			this->store(i.ret, reg);
		}
		else if (i.op == TAC::RETURN)
		{
			Operand reg = {GET_SUB_REG(RAX, i.operation_size), i.operation_sign};
			this->load(reg, i.oper1);
			this->leave();
		}
		else if (i.op == TAC::PARAM)
			this->add_param(i);
		else if (i.op == TAC::CALL)
			this->call(i);
		else if (i.op == TAC::DEREFERENCE)
		{
			this->load(GET_SUB_REG(RAX, size_of(i.oper1)), i.oper1);

			auto op = alloc_temporary(PTR_SIZE);
			move(op, GET_SUB_REG(RAX, size_of(i.oper1)));
			size_t s = symbolTable.size_of(this->function.tac->temps[get<int>(i.ret)]);
			this->add_mapping(i.ret, Operand(new Indirection{op, 0, s, 0}));
		}
		else if (i.op == TAC::ADDRESS)
		{
			auto location = get_address_location(i.oper1);
			assert(location.type == Operand::INDIRECT);
			if (location.indirection->base.type == Operand::INDIRECT)
			{
				this->load(GET_SUB_REG(RAX, PTR_SIZE), location.indirection->base);
				location.indirection->base = GET_SUB_REG(RAX, PTR_SIZE);
			}
			this->put_instruction("lea", GET_SUB_REG(RAX, PTR_SIZE), location);
			this->store(i.ret, GET_SUB_REG(RAX, PTR_SIZE));
		}

		this->gen.put("");
	}

	auto	FunctionGenerator::get_last_usage_pqueue() const
	{
		std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::function<bool(const std::pair<int, int> &, const std::pair<int, int> &)>> ret([](const std::pair<int, int> &l, const std::pair<int, int> &r) -> bool {return r.first < l.first;});
		const auto &v = function.tac->get_last_usages();
		for (size_t i = 0; i < v.size(); i++)
		{
//			std::cout << "last usage of t" << i << " at line " << v[i] << std::endl;
			ret.emplace(v[i], i);
		}
		return ret;
	}

	auto	FunctionGenerator::get_labels_pqueue() const
	{
		std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::function<bool(const std::pair<int, int> &, const std::pair<int, int> &)>> ret([](const std::pair<int, int> &l, const std::pair<int, int> &r) -> bool {return r.first < l.first;});
		const auto &v = function.tac->labels;
		for (size_t i = 0; i < v.size(); i++)
		{
//			std::cout << "label l" << i << " at line " << v[i] << std::endl;
			ret.emplace(v[i], i);
		}
		return ret;
	}

	void FunctionGenerator::map(const TAC::Address &a, const CodeGeneration::Operand &o) { //todo duplicate with add_mapping?
		if(this->map_variables[a].type == Operand::REGISTER)
		{
			this->allocated_registers.erase(this->map_variables[a].reg);
			this->available_registers.insert(this->map_variables[a].reg);
		}
		this->map_variables[a] = o;
		if (o.type == Operand::REGISTER)
		{
			this->available_registers.erase(o.reg);
			this->allocated_registers.insert({o.reg, this->map_variables.find(a)->first});
		}
	}

	void FunctionGenerator::map_params() {
		int i = 0;
		for (auto *o : this->function.params)
		{
//			std::cout << "param: " << o->name << " " << o << std::endl;
			this->map_variables[o] = Operand(new Indirection{GET_SUB_REG(RBP, REG_SIZE), 0, symbolTable.size_of(o->type), (ssize_t)(REG_SIZE * (2 + i))});
			i++;
		}
	}

	void FunctionGenerator::translate() {
		this->init_registers();
		this->map_variables["@rbp"] = GET_SUB_REG(RBP, REG_SIZE); // todo: try to remove and see if it break
		this->map_params();
		const auto & instructions = function.tac->get_instructions();
		auto lupq = get_last_usage_pqueue();
		auto lpq = get_labels_pqueue();
		this->put_name();
		this->enter();
//		this->spill_params();
		for ( size_t i = 0; i < instructions.size(); i++)
		{
			while (!lpq.empty() && lpq.top().first <= i)
			{
				if (!map_labels.contains(lpq.top().second))
					map_labels[lpq.top().second] = gen.get_label();
				this->gen.put(".L" + std::to_string(map_labels[lpq.top().second]) + ":");
				lpq.pop();
			}
			std::cout.flush();
			this->translate_instruction(instructions[i]);
			while (!lupq.empty() && lupq.top().first < i)
			{
//				printf("free %d\n", lupq.top().second);
//				this->free_temp(lupq.top().second); todo
				lupq.pop();
			}
		}
		while (!lpq.empty())
		{
			if (!map_labels.contains(lpq.top().second))
				map_labels[lpq.top().second] = gen.get_label();
			this->gen.put(".L" + std::to_string(map_labels[lpq.top().second]) + ":");
			lpq.pop();
		}
		this->leave();
	}

	void FunctionGenerator::enter() {
		this->put_instruction("push", GET_SUB_REG(RBP, REG_SIZE));
		this->put_instruction("mov", GET_SUB_REG(RBP, REG_SIZE), GET_SUB_REG(RSP, REG_SIZE));
		this->put_instruction("sub", GET_SUB_REG(RSP, REG_SIZE), this->function.frame_size);
		this->put_instruction("push", GET_SUB_REG(RBX, REG_SIZE));
		this->frame_size += REG_SIZE + this->function.frame_size;
	}

	void FunctionGenerator::leave() {
		this->put_instruction("pop", GET_SUB_REG(RBX, REG_SIZE));
		this->put_instruction("mov", GET_SUB_REG(RSP, REG_SIZE), GET_SUB_REG(RBP, REG_SIZE));
		this->put_instruction("pop", GET_SUB_REG(RBP, REG_SIZE));
		this->put_instruction("ret");
	}

	void FunctionGenerator::put_name() {
	}


	FileGenerator::FileGenerator(std::ostream &o) : out(o), line(0) {}

	void FileGenerator::put_constant(const SymbolTable::Constant *c) {
		if (holds_alternative<uintmax_t>(c->value))
			return;
		this->put(".section .rodata");
		this->put(".LC" + std::to_string(c->id) + ":");
		switch (c->value.index())
		{
			case 0:
				return this->put( ".long " + std::to_string(*(long int *)&get<1>(c->value)));
			case 2:
				return this->put( ".string " + get<2>(c->value));
			default:
				throw std::runtime_error("shouldn't be reached");
		}
	}

	void FileGenerator::put_ordinary(const SymbolTable::Ordinary *o) {
		int size = symbolTable.size_of(o->type);
		// todo: align
		if (!o->init)
			this->put(".zero " + std::to_string(size));
		else
		{
			auto init = o->init.value();
			switch (init->value.index())
			{
				case 0:
					this->put(".double " + std::to_string(get<0>(init->value)));
					break;
				case 1:
					this->put(".long " + std::to_string(get<1>(init->value)));
					break;
				case 2:
					this->put(".long .LC" + std::to_string(init->id));
					break;
			}
		}
	}

	void FileGenerator::generate() {
		this->put(".intel_syntax noprefix");
		this->put(".text");
		for (auto &sym : symbolTable.symbols)
		{
			if (sym.visibility == SymbolTable::Symbol::NONE)
				continue;

//			std::cout << sym.name << std::endl;
			if (holds_alternative<SymbolTable::Function*>(sym.value))
				this->put(".text");
			else
				this->put(".data"); // todo: bss

			if (sym.size)
				this->put(".size " + sym.name + ", " + std::to_string(sym.size));

			//todo: asm type:
			if (sym.visibility == SymbolTable::Symbol::GLOBAL)
			{
				this->put(".globl " + sym.name);
			}
			else
				this->put(".local " + sym.name);
			this->put(sym.name + ":");
			switch (sym.value.index())
			{
				case 0:
				{
//					std::cout << "translating function " << sym.name << std::endl;
//					std::cout << "============= TAC CODE ==============" << std::endl;
//					get<SymbolTable::Function*>(sym.value)->tac->print();
//					std::cout << "========== END OF TAC CODE ==========" << std::endl;
					CodeGeneration::FunctionGenerator(*get<SymbolTable::Function*>(sym.value), *this).translate();
				}
				break;
				case 1:
				{
					this->put_ordinary(get<SymbolTable::Ordinary*>(sym.value));
				}
				break;
			}
		}
		for (auto &c : symbolTable.constants)
			this->put_constant(&c);
	}

	void FileGenerator::put(const std::string &s) {
		out << s << std::endl;
		line++;
	}

	int FileGenerator::get_lineno() const {
		return this->line;
	}
}
