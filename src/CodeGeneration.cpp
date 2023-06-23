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
		const auto &l_base = get_address_location(i.base);
		const auto &l_index = get_address_location(i.index);
		if	(l_base.type != Operand::REGISTER ||
			(l_index.type != Operand::IMMEDIATE && l_index.type != Operand::REGISTER))
			throw std::runtime_error("shouldn't be reached");
		return "[" + operand_to_string(l_base) + "+" + operand_to_string(l_index) + "*" + std::to_string(i.scale) + "+" + std::to_string(i.displacement) + "]";
	}

	std::string FunctionGenerator::operand_to_string(const Operand &a) {
		switch (a.type)
		{
			case Operand::REGISTER:
				return registers_strings[static_cast<int>(a.reg)];
			case Operand::INDIRECT: {
				const auto &l_base = get_address_location(a.indirection->base);
				const auto &l_index = get_address_location(a.indirection->index);
				if (l_base.type != Operand::REGISTER ||
					(l_index.type != Operand::IMMEDIATE &&
					l_index.type != Operand::REGISTER))
					throw std::runtime_error("shouldn't be reached");
				return operand_size_str[a.indirection->scale] + " PTR " + indirection_to_string(*a.indirection);
			}
			case Operand::DIRECT:
				return a.label;
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
		this->available_registers = decltype(this->available_registers)(registers.begin(), registers.end(), &compare_reg);
	}

	void FunctionGenerator::spill(const CodeGeneration::Register &r) {
		auto it = this->allocated_registers.find(r);
		if (it == this->allocated_registers.end())
			return;
		this->put_instruction("push", r);
		this->frame_size += 8;
		this->map_variables[it->second] = Operand(new Indirection{"@rbp", NULL_CONSTANT, 8, -(ssize_t)this->frame_size});
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

	Operand FunctionGenerator::temp_to_operand(int temp) {
		Operand mapping;
		try { // map on register
			Register r = GET_SUB_REG(alloc_register(), symbolTable.size_of(this->function.tac->temps[temp]));
			printf("size: %zu %s\n", symbolTable.size_of(this->function.tac->temps[temp]), registers_strings[int(r)]);
			mapping.type = Operand::REGISTER;
			mapping.reg = r;
		}
		catch (std::exception&) // no more registers, map on stack
		{
			this->put_instruction("push", 0);
			mapping.type = Operand::INDIRECT;
			auto *i = new Indirection {"@rbp", NULL_CONSTANT, symbolTable.size_of(this->function.tac->temps[temp]), -(ssize_t)this->frame_size};
			this->frame_size += i->scale;
			mapping.indirection = std::shared_ptr<Indirection>(i);
		}
		return mapping;
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
				ret = Operand(new Indirection{"@rbp", NULL_CONSTANT, symbolTable.size_of(get<SymbolTable::Ordinary *>(addr)->type), -get<SymbolTable::Ordinary *>(addr)->offset});
				break;
			case 3:
			{
				const auto &constant = get<const SymbolTable::Constant *>(addr);
				switch (constant->value.index()) {
					case 0:
						ret = {".LC" + std::to_string(constant->id) + "@GOTPCREL[rip]"};
				break;
					case 1:
						ret = {get<uintmax_t>(constant->value)};
				break;
					case 2:
						ret = {".LC" + std::to_string(constant->id) + "@GOTPCREL[rip]"};
					break;
				}
				break;
			}
			case 4:
				ret = {".L" + std::to_string(get<4>(addr))};
				break;
			case 5:
				ret = {get<5>(addr) + "@GOTPCREL[rip]"};
				break;
		}
		if (ret.type == Operand::REGISTER)
			this->allocated_registers.insert({ret.reg, addr});
		return ret;
	}

	Operand FunctionGenerator::get_address_location(const TAC::Address &addr) {
		std::cout << "enter get_address_location" << std::endl;
		if (holds_alternative<std::string>(addr))
			std::cout << "get addr for " << get<std::string>(addr) << std::endl;
		if (holds_alternative<SymbolTable::Ordinary *>(addr))
			std::cout << "get addr for " << get<SymbolTable::Ordinary *>(addr)->name << " " << get<SymbolTable::Ordinary *>(addr) << std::endl;
		if (auto it = this->map_variables.find(addr); it != this->map_variables.end())
			return it->second;
		std::cout << "zbeub " << std::endl;
		return this->map_variables.insert({addr, address_to_operand(addr)}).first->second;
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
		Register left = GET_SUB_REG(EAX, size_of(i.ret)), right = GET_SUB_REG(EBX, size_of(i.ret));

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
				put_instruction("imul", left, right);
				// todo: sign
				break;
			case TAC::DIV:
				put_instruction("mov", EAX, left);
				put_instruction("div", right);
				put_instruction("mov", left, EAX);
				break;
			case TAC::MOD:
				put_instruction("mov", EAX, left);
				put_instruction("div", right);
				put_instruction("mov", left, EDX);
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
		}
		store(i.ret, left);
	}

	void FunctionGenerator::translate_jump(const TAC::Instruction &i) {
		if (this->function.tac->labels[get<TAC::Label>(i.ret)] < 0)
			throw std::runtime_error("no dst for label l" + std::to_string(get<TAC::Label>(i.ret)));

		if (i.op != TAC::JUMP) {
			Register left = EAX, right = EBX;
			load(left, i.oper1);
			load(right, i.oper2);
			this->put_instruction("cmp", left, right);
		}
		std::string opcodes[] = {
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
		if (param_pos < sizeof(param_registers) / sizeof (*param_registers))
		{
			this->spill(param_registers[param_pos]);
			CLEAN_REG(param_registers[param_pos]);
			this->load(GET_SUB_REG(param_registers[param_pos], size_of(get_address_location(i.oper1))), i.oper1);
			param_pos++;
		}
		else
			this->stack_args.push(get_address_location(i.oper1));
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
				return 8; // todo
			case Operand::IMMEDIATE:
				return 8; // todo
			default:
				throw std::runtime_error("shouldn't be reached");
		}
	}

	size_t	FunctionGenerator::size_of(const TAC::Address &o)
	{
		return this->size_of(get_address_location(o));
	}

	void FunctionGenerator::call(const TAC::Instruction &i) {
		this->load(RAX, i.oper1);
		size_t padding = (16 - (this->frame_size % 16)) % 16;
		if (padding)
			this->put_instruction("sub", RSP, padding); // todo
		for (;!this->stack_args.empty(); this->stack_args.pop())
			this->put_instruction("push", this->stack_args.top());
		this->put_instruction("call", RAX);
		if (padding)
			this->put_instruction("add", RSP, padding);
//		for (;param_pos > 0; param_pos--)
//			this->put_instruction("pop", param_registers[param_pos - 1]); //todo: reload spilled
		this->param_pos = 0;
		this->store(i.ret, RAX);
	}

	// load an address in RAX or RBX
	void FunctionGenerator::load(const Register &reg, const TAC::Address &addr) {
		const auto &location = get_address_location(addr);
		if (location.type == Operand::INDIRECT)
		{

			const auto &l_base = get_address_location(location.indirection->base);
			const auto &l_index = get_address_location(location.indirection->index);
			if (l_base.type == Operand::INDIRECT || l_index.type == Operand::INDIRECT)
			{
				Indirection i = *location.indirection;
				put_instruction("mov", GET_SUB_REG(reg, 8), l_base); // todo:  32bit
				this->map(location.indirection->base, GET_SUB_REG(reg, 8));
				put_instruction("mov", GET_SUB_REG(reg, this->size_of(location)), location);
				// todo: redo
			}
			else
				put_instruction("mov", GET_SUB_REG(reg, this->size_of(location)), location);
		}
		else
			put_instruction("mov", GET_SUB_REG(reg, this->size_of(location)), location);
	}

	void FunctionGenerator::store(const TAC::Address &address, const CodeGeneration::Register &reg) {
		const auto &location = get_address_location(address);
		if (location.type == Operand::INDIRECT)
		{
			const auto &l_base = get_address_location(location.indirection->base);
			const auto &l_index = get_address_location(location.indirection->index);
			if (l_base.type == Operand::INDIRECT || l_index.type == Operand::INDIRECT)
			{
				// todo
			}
			else
				put_instruction("mov", location, GET_SUB_REG(reg, this->size_of(location)));
		}
		else
			put_instruction("mov", location, GET_SUB_REG(reg, this->size_of(location)));
	}

	void FunctionGenerator::move(const TAC::Address &dst, const TAC::Address &src) {

	}

	void FunctionGenerator::translate_instruction(const TAC::Instruction& i) {
		this->instruction_to_asm.emplace_back(this->gen.get_lineno());
		if (i.is_bop())
			this->translate_bop(i);
		if (i.is_jump())
			this->translate_jump(i);
		if (i.op == TAC::ASSIGN)
			put_instruction("mov", get_address_location(i.ret), get_address_location(i.oper1));
		if (i.op == TAC::RETURN)
		{
			put_instruction("mov", GET_SUB_REG(RAX, size_of(i.oper1)), get_address_location(i.oper1));
			this->leave();
		}
		if (i.op == TAC::PARAM)
			this->add_param(i);
		if (i.op == TAC::CALL)
			this->call(i);
		if (i.op == TAC::DEREFERENCE)
			this->map(i.ret, Operand(new Indirection{i.oper1, NULL_CONSTANT, size_of(i.oper1), 0}));

	}

	auto	FunctionGenerator::get_last_usage_pqueue() const
	{
		std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::function<bool(const std::pair<int, int> &, const std::pair<int, int> &)>> ret([](const std::pair<int, int> &l, const std::pair<int, int> &r) -> bool {return r.first < l.first;});
		const auto &v = function.tac->get_last_usages();
		for (size_t i = 0; i < v.size(); i++)
		{
			std::cout << "last usage of t" << i << " at line " << v[i] << std::endl;
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
			std::cout << "label l" << i << " at line " << v[i] << std::endl;
			ret.emplace(v[i], i);
		}
		return ret;
	}

	void FunctionGenerator::map(const TAC::Address &a, const CodeGeneration::Operand &o) {
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
			std::cout << "param: " << o->name << " " << o << std::endl;
			if (i < sizeof(param_registers) / sizeof (*param_registers))
			{
				std::cout << "register " << registers_strings[int(param_registers[i])] << std::endl;
				this->map_variables[o] = param_registers[i];
				this->available_registers.erase(param_registers[i]);
				this->allocated_registers.insert({param_registers[i], this->map_variables.find(o)->first});
			}
			else this->map_variables[o] = Operand(new Indirection{"@rbp", NULL_CONSTANT, 8, -(ssize_t)(8 * (1 + i - sizeof(param_registers) / sizeof (*param_registers)))});
			i++;
		}
	}

	void FunctionGenerator::translate() {
		this->init_registers();
		this->map_variables["@rbp"] = RBP;
		this->map_params();
		const auto & instructions = function.tac->get_instructions();
		auto lupq = get_last_usage_pqueue();
		auto lpq = get_labels_pqueue();
		this->put_name();
		this->enter();
		this->spill_params();
		for ( size_t i = 0; i < instructions.size(); i++)
		{
			while (!lpq.empty() && lpq.top().first <= i)
			{
				this->gen.put(".L" + std::to_string(lpq.top().second) + ":");
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
			this->gen.put(".L" + std::to_string(lpq.top().second) + ":");
			lpq.pop();
		}
		this->leave();
	}

	void FunctionGenerator::spill_params() {
		for (int i = 0; i < this->function.params.size() && i < sizeof(param_registers) / sizeof (*param_registers); i++)
			spill(param_registers[i]);
	}

	void FunctionGenerator::enter() {
		this->put_instruction("push", RBP);
		this->put_instruction("mov", RBP, RSP);
		this->put_instruction("sub", RSP, this->frame_size);
		this->put_instruction("push", RBX);
		this->frame_size += 8;
	}

	void FunctionGenerator::leave() {
		this->put_instruction("pop", RBX);
		this->put_instruction("mov", RSP, RBP);
		this->put_instruction("pop", RBP);
		this->put_instruction("ret");
	}

	void FunctionGenerator::put_name() {
		this->gen.put(".globl " + this->function.name);
//		this->gen.put(".type " + this->function.tac->function.name + ", @function");
		this->gen.put(this->function.name + ":");
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

	void FileGenerator::generate() {
		this->put(".intel_syntax noprefix");
		this->put(".text");
		for (auto &sym : symbolTable.symbols)
		{
			std::cout << sym.name << std::endl;
			if (holds_alternative<SymbolTable::Function*>(sym.value))
			{
				get<SymbolTable::Function*>(sym.value)->tac->print();
				CodeGeneration::FunctionGenerator(*get<SymbolTable::Function*>(sym.value), *this).translate();
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
