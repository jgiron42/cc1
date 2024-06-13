#include "SymbolTable.hpp"
#include <iostream>

SymbolTable::Constant::Constant(const std::string &val) : type(CTYPE_CHAR_PTR), value(val) {this->set_id();}

SymbolTable::SymbolTable() {
	this->block_stack.push(&this->global);
	block_pos_stack.push(0);
}

void SymbolTable::enter_prototype() {
	this->prototype = true;
}

void SymbolTable::exit_prototype() {
	this->prototype = false;
}

void SymbolTable::enter_function(const Function &f) {
	assert(!current_function);
	current_function = &this->functions.emplace_back(f);
	this->symbols.push_back({
		.name = current_function->name,
		.visibility = Symbol::GLOBAL, // todo
		.value = current_function
	});
}

void SymbolTable::exit_function() {
	assert(current_function);
	current_function = nullptr;
}

int SymbolTable::enter_block() {
//	std::cout << "[enter block]";
	if (block_stack.top()->blocks.size() >= this->block_pos_stack.top())
		block_stack.top()->blocks.emplace_back();
	this->block_stack.push(&this->block_stack.top()->blocks[this->block_pos_stack.top()]);
	this->block_pos_stack.top()++;
	this->block_pos_stack.push(0);
	return this->block_pos_stack.top() - 1;
}

void SymbolTable::exit_block() {
//	std::cout << "[exit block]";
	this->block_stack.pop();
	this->block_pos_stack.pop();
}

bool SymbolTable::insert_ordinary(const std::string &name, Ordinary o) {
//	std::cout << "[insert " << name << ", type: ";
//	this->print_type(o.type);
//	std::cout << "]";

	if (o.storage == Ordinary::Storage::UNDEFINED)
	{
		if (holds_alternative<Types::FunctionType>(o.type.type))
			o.storage = Ordinary::Storage::EXTERN;
		else if (current_function)
			o.storage = Ordinary::Storage::AUTO;
		else if (prototype)
			o.storage = Ordinary::PARAM;
		else
			o.storage = Ordinary::Storage::GLOBAL;
	}
	if (o.storage == Ordinary::STATIC)
		o.name = name + "." + std::to_string(this->functions.size());
//todo check storage

	Scope *scope;
	if (this->block_stack.empty())
		scope = &this->global.scope;
	else
		scope = &this->block_stack.top()->scope;
	auto [it, ret] = scope->objects.insert(std::make_pair(name, o));
	if (!ret)
		return false;

	if (current_function)
	{
//		if (prototype)
//		{
//			assert(it->second.storage == Ordinary::UNDEFINED); // todo
//			it->second.storage = SymbolTable::Ordinary::PARAM;
//		} else
		if (it->second.storage == Ordinary::Storage::AUTO)
		{
			current_function->frame_size += size_of(it->second.type);
			it->second.offset = current_function->frame_size;
		}
		else
			this->add_symbol(o.name, it->second);
	}
	else if (!prototype)
	{
		this->add_symbol(o.name, it->second);
	}

	return true;
}

bool	SymbolTable::insert_function(const std::string &name, Ordinary o)
{
	if (o.storage == Ordinary::Storage::UNDEFINED)
		o.storage = Ordinary::EXTERN;

	auto [it, ret] = this->global.scope.objects.insert(std::make_pair(name, o));
	if (!ret)
		return false;
	this->add_symbol(name, it->second);
	return true;
}

bool SymbolTable::is_typename(const std::string &name) {
	if (auto *o = this->retrieve_ordinary(name); o)
		return o->storage == Ordinary::TYPEDEF;
	return false;
}

SymbolTable::Ordinary	*SymbolTable::retrieve_ordinary(const std::string &name) {
	std::map<std::string, Ordinary>::iterator it;
	auto s = this->block_stack;
	while(!s.empty() && (it = s.top()->scope.objects.find(name)) == s.top()->scope.objects.end())
		s.pop();
	if (!s.empty())
		return &(it->second);
	return nullptr;
}

bool	SymbolTable::declare_tag(const std::string &name, Types::Tag::Type type) {
	return this->block_stack.top()->scope.tags.insert({name, Types::Tag{type}}).second;
}

bool	SymbolTable::assign_tag(const std::string &name, Types::StructOrUnion sou)
{
	auto it = this->block_stack.top()->scope.tags.find(name);
	assert (it != this->block_stack.top()->scope.tags.end());
	if (it->second.type == Types::Tag::UNION ^ sou.is_union)
		return false; //operation is different
	it->second.declaration = sou;
	return true;
}

bool	SymbolTable::assign_tag(const std::string &name, Types::Enum e)
{
	auto it = this->block_stack.top()->scope.tags.find(name);
	assert (it != this->block_stack.top()->scope.tags.end());
	it->second.declaration = e;
	return true;
}

Types::Tag	*SymbolTable::retrieve_tag(const std::string &name) {
	std::map<std::string, Types::Tag>::iterator it;
	auto s = this->block_stack;
	while(!s.empty() && (it = s.top()->scope.tags.find(name)) == s.top()->scope.tags.end())
		s.pop();
	if (it != block_stack.top()->scope.tags.end())
		return &(it->second);
	return nullptr;
}

bool SymbolTable::contain_tag(const std::string &name) {
	return this->retrieve_tag(name);
}

size_t SymbolTable::size_of(const Types::CType &t) {
	switch (t.type.index())
	{
		case 0: // PlainType
		{
			switch (get<0>(t.type).base)
			{
				case Types::PlainType::VOID:
					return 1;
				case Types::PlainType::CHAR:
					return CHAR_SIZE;
				case Types::PlainType::SHORT_INT:
					return SHORT_INT_SIZE;
				case Types::PlainType::INT:
					return INT_SIZE;
				case Types::PlainType::LONG_INT:
					return LONG_INT_SIZE;
				case Types::PlainType::FLOAT:
					return FLOAT_SIZE;
				case Types::PlainType::DOUBLE:
					return DOUBLE_SIZE;
				case Types::PlainType::LONG_DOUBLE:
					return LONG_DOUBLE_SIZE;
			}
			break;
		}
		case 1: // TagName
			throw std::runtime_error("Tags are unsupported");
			//todo: tags
			break;
		case 2: // Pointer
			return PTR_SIZE;
		case 3: // FunctionType
			return PTR_SIZE; // todo warn?
		case 4: // Array
			if (!get<4>(t.type).size)
				throw std::runtime_error("Can't take sizeof an incomplete array"); //todo: better
			return get<4>(t.type).size.value() * this->size_of(*get<4>(t.type).value_type);
		case 5: // Typename
		{
			auto *type = this->retrieve_ordinary(get<5>(t.type).name);
			if (!type)
				throw std::runtime_error("type " + get<5>(t.type).name + " does not exist"); //todo: better
			throw std::runtime_error("not implemented yet " + std::string(__FILE__ +  __LINE__));
		}
		default:
			throw std::runtime_error("shouldn't be reached");
	}
}

SymbolTable::Function &SymbolTable::get_current_function() {
	return *this->current_function;
}

void SymbolTable::print_type(const Types::CType &t) {
	const Types::CType *p = &t;
	while (p) {
		if (p->qualifier & Types::CType::CONST)
			std::cout << "const ";
		if (p->qualifier & Types::CType::VOLATILE)
			std::cout << "volatile ";
		switch (p->type.index()) {
			case 0:
				std::cout << "plain type";
				p = NULL;
				break;
			case 1:
				std::cout << "tag " << get<1>(p->type).name;
				p = NULL;
				break;
			case 2:
				std::cout << "pointer to ";
				p = get<2>(p->type).pointed_type.get();
				break;
			case 3:
				std::cout << "function returning ";
				p = get<3>(p->type).return_type.get();
				break;
			case 4:
				std::cout << "array of ";
				if (get<4>(p->type).size)
					std::cout << "size " << get<4>(p->type).size.value() << " of ";
				p = get<4>(p->type).value_type.get();
				break;
			case 5:
				std::cout << "typename " << get<5>(p->type).name;
				p = NULL;
				break;
		}
	}
	std::cout << std::endl;
}

void SymbolTable::add_symbol(const std::string &name, SymbolTable::Ordinary &ordinary) {

		Symbol sym { name };

		sym.read_only = ordinary.type.qualifier & Types::CType::CONST;

		if (ordinary.storage == Ordinary::STATIC)
			sym.visibility = Symbol::LOCAL;
		else if (ordinary.storage == Ordinary::GLOBAL)
			sym.visibility = Symbol::GLOBAL;
		else
			sym.visibility = Symbol::NONE;

		sym.value = &ordinary;

		sym.size = this->size_of(ordinary.type);

		this->symbols.emplace_back(sym);
}

SymbolTable symbolTable;