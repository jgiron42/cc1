
#ifndef CC1_POC_SYMBOLTABLE_HPP
#define CC1_POC_SYMBOLTABLE_HPP

#include <optional>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <list>
#include <stack>
#include <string>
#include <stdexcept>
#include <variant>
#include <assert.h>

#define CTYPE_UNSIGNED_LONG_INT Types::CType{Types::PlainType{Types::PlainType::LONG_INT, false}}
#define CTYPE_LONG_INT Types::CType{Types::PlainType{Types::PlainType::LONG_INT, true}}
#define CTYPE_INT Types::CType{Types::PlainType{Types::PlainType::INT, true}}
#define CTYPE_LONG_DOUBLE Types::CType{Types::PlainType{Types::PlainType::LONG_DOUBLE}}
#define CTYPE_CHAR_PTR Types::CType{Types::Pointer{std::shared_ptr<Types::CType>(new Types::CType{Types::PlainType{Types::PlainType::CHAR, true}})}}

#define CHAR_SIZE 1
#define SHORT_INT_SIZE 2
#define INT_SIZE 4
#define LONG_INT_SIZE 8
#define FLOAT_SIZE 4
#define DOUBLE_SIZE 8
#define LONG_DOUBLE_SIZE 8
#define PTR_SIZE 8
#define CONST(n) (symbolTable.new_constant(n))
#define NULL_CONSTANT CONST(0)
#include "Types.hpp"

namespace TAC {
	class TacFunction;
}

class SymbolTable {
public:

	struct Constant
	{
		Constant() {this->set_id();};
		template <typename T,std::enable_if_t<std::is_integral<T>::value, bool> = true>
		Constant(T val) : type(CTYPE_LONG_DOUBLE) {
			value.emplace<uintmax_t>(val);
			this->set_id();
		}

		template <typename T,std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
		Constant(T val) : type(CTYPE_LONG_INT) {
			value.emplace<long double>(val);
			this->set_id();
		}
		Constant(const std::string &);

		friend bool operator<(const Constant &l, const Constant &r)
		{
			return l.value < r.value;
		}
		void set_id()
		{
			static int current = 0;
			this->id = current++;
		}

		Types::CType	type;
		std::variant<
				long double,
				uintmax_t,
				std::string
		>				value;
		int				id;
	};

	struct Ordinary {
		enum Storage {
			UNDEFINED,
			TYPEDEF,
			STATIC,
			AUTO,
			REGISTER,
			EXTERN,
			PARAM,
		};
		Storage					storage;
		Types::CType			type;
		std::string				name;
		int						offset;
		std::optional<Constant>	init;
		// location;
	};

	struct Label {
	};

	struct Scope {
		std::map<std::string, Types::Tag> tags;
		std::map<std::string, Ordinary> objects;
	};

	struct Block {
		Scope scope;
		std::vector<Block> blocks;
	};

	struct Function {
		Types::CType						type;
		std::string							name;
		std::map<std::string, Label>		labels;
		Block								block;
		int									frame_size;
		TAC::TacFunction					*tac;
		std::list<Ordinary*>				params;
	};

	struct Symbol {
		enum Visibility {
			NONE,
			GLOBAL,
			LOCAL,
		};
		std::string	name;
		bool		read_only;
		Visibility	visibility;
		std::variant<
			Function*,
			Ordinary*
		> value;
	};


	Block global;
//	std::map<std::string, Function> functions;
	std::deque<Function> functions;

	Function												*current_function;
	bool													prototype;
	std::stack<Block*>										block_stack; // represent the hierarchy of block
	std::stack<int>											block_pos_stack; // represent the next sub-block inside each block of the current tree
	std::set<Constant>										constants;

	std::vector<Symbol>										symbols;
public:
	SymbolTable();

	void								enter_function(const Function &);
	void								exit_function();


	void	enter_prototype();
	void	exit_prototype();

	int enter_block();
	void exit_block();

//	void insert_label();
//	void insert_tag();
	bool								insert_ordinary(const std::string &name, Ordinary);
	bool								insert_function(const std::string &name, Ordinary);
	Ordinary*							retrieve_ordinary(const std::string &name);

	bool								is_typename(const std::string &name);

	bool								declare_tag(const std::string &name, Types::Tag::Type);
	bool								assign_tag(const std::string &name, Types::StructOrUnion);
	bool								assign_tag(const std::string &name, Types::Enum);
	Types::Tag*								retrieve_tag(const std::string &name);
	bool								contain_tag(const std::string &name);

	size_t									size_of(const Types::CType &);
	template <typename ...Args>
	const Constant *new_constant(Args &&... args)
	{
		return &*(this->constants.emplace(args...).first);
	}
	Function							&get_current_function();

	void 								print_type(const Types::CType &);

	void add_symbol(const std::string &name, Ordinary &ordinary);
};

extern SymbolTable symbolTable;

#endif
