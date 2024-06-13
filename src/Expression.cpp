#include "Expression.hpp"
#include <queue>
#include "Expression.hpp"

namespace Expression {

	bool	type_equivalence(const Types::CType &t1, const Types::CType &t2, bool ignore_qualifier = true)
	{
		std::queue<std::pair<const Types::CType*, const Types::CType*>> q;
		q.emplace(&t1, &t2);

		while (!q.empty())
		{
			auto [a, b] = q.front();
			if (a->type.index() != b->type.index())
				return false;
			if (!ignore_qualifier && a->qualifier != b->qualifier)
				return false;
			switch (a->type.index())
			{
				case 0: // PlainType
				if (get<Types::PlainType>(a->type) != get<Types::PlainType>(b->type))
				{
					if (get<Types::PlainType>(a->type).base != Types::PlainType::VOID || get<Types::PlainType>(b->type).base != Types::PlainType::VOID) // todo
						return false;
				}
				break;
				case 1: // TagName
				if (get<Types::TagName>(a->type) != get<Types::TagName>(b->type))
					return false;
				break;
				case 2: // Pointer
					q.emplace(get<Types::Pointer>(a->type).pointed_type.get(),get<Types::Pointer>(b->type).pointed_type.get());
				break;
				case 3: // FunctionType
					if (get<Types::FunctionType>(a->type).parameters.size() != get<Types::FunctionType>(b->type).parameters.size())
						return false;
					q.emplace(get<Types::FunctionType>(a->type).return_type.get(), get<Types::FunctionType>(b->type).return_type.get());
					for (auto it1 = get<Types::FunctionType>(a->type).parameters.begin(), it2 = get<Types::FunctionType>(b->type).parameters.begin();; it1++, it2++)
					{
						if (it1 == get<Types::FunctionType>(a->type).parameters.end())
						{
							if (it2 == get<Types::FunctionType>(b->type).parameters.end())
								break;
							else
								return false;
						}
						q.emplace(&*it1, &*it2);
					}
				break;
				case 4: // Array
					if (get<Types::Array>(a->type).size != get<Types::Array>(b->type).size)
						return false;
					q.emplace(get<Types::Array>(a->type).value_type.get(),get<Types::Array>(b->type).value_type.get());
				break;
				case 5: // Typename
					if (get<Types::Typename>(a->type).name != get<Types::Typename>(b->type).name)
						return false;
				break;
			}
			q.pop();
		}
		return true;
	}

	bool	is_null(const Node &n)
	{
		return (n.operation == Expression::CONSTANT && holds_alternative<uintmax_t>(n.constant.value()->value) && !get<uintmax_t>(n.constant.value()->value));
	}

	bool	is_integral(const Types::CType &t)
	{
		if (!holds_alternative<Types::PlainType>(t.type))
			return false;
		const auto & pt = get<Types::PlainType>(t.type);
		return pt.base == Types::PlainType::CHAR
			|| pt.base == Types::PlainType::INT
			|| pt.base == Types::PlainType::SHORT_INT
			|| pt.base == Types::PlainType::LONG_INT;
	}

	bool	is_floating(const Types::CType &t)
	{
		if (!holds_alternative<Types::PlainType>(t.type))
			return false;
		const auto & pt = get<Types::PlainType>(t.type);
		return pt.base == Types::PlainType::FLOAT
			|| pt.base == Types::PlainType::DOUBLE
			|| pt.base == Types::PlainType::LONG_DOUBLE;
	}

	bool	is_void(const Types::CType &t)
	{
		if (!holds_alternative<Types::PlainType>(t.type))
			return false;
		const auto & pt = get<Types::PlainType>(t.type);
		return pt.base == Types::PlainType::VOID;
	}

	bool	is_arithmetic(const Types::CType &t)
	{
		return is_integral(t) || is_floating(t);
	}

	bool	is_pointer(const Types::CType &t)
	{
		return holds_alternative<Types::Pointer>(t.type);
	}

	bool	is_scalar(const Types::CType &t)
	{
		return is_arithmetic(t) || is_pointer(t);
	}

	bool	is_struct(const Types::CType &t)
	{
		return holds_alternative<Types::TagName>(t.type)
			&& get<Types::TagName>(t.type).type == Types::Tag::STRUCT;
	}

	bool	is_aggregate(const Types::CType &t)
	{
		return is_struct(t)
			|| holds_alternative<Types::Array>(t.type);
	}

	std::optional<Types::CType> max(const Types::CType &t1, const Types::CType &t2)
	{
		if (type_equivalence(t1,t2))
			return t1;
			return t1;
	}

	bool			are_compatible(const Types::CType &t1, const Types::CType &t2)
	{
		if (is_pointer(t1) && is_pointer(t2))
			return are_compatible(*get<Types::Pointer>(t1.type).pointed_type, *get<Types::Pointer>(t2.type).pointed_type);
		else if (holds_alternative<Types::FunctionType>(t1.type))
		{
			if (!holds_alternative<Types::FunctionType>(t2.type))
				return false;
			auto &f1 = std::get<Types::FunctionType>(t1.type);
			auto &f2 = std::get<Types::FunctionType>(t2.type);
			if (!are_compatible(*f1.return_type, *f2.return_type))
				return false;
			auto it1 = f1.parameters.begin(), it2 = f2.parameters.begin();
			for (; it1 != f1.parameters.end() && it2 != f2.parameters.end(); it1++, it2++)
				if (!are_compatible(*it1, *it2))
					return false;
			if (it1 != f1.parameters.end() || it2 != f2.parameters.end())
				return false;
			return true;
		}
		else
		{
			return t1.type == t2.type;// todo
		}
	}

	bool			is_convertible_to(const Types::CType &dst, const Types::CType &src)
	{
//		if (!(~(dst.qualifier ^ src.qualifier) | dst.qualifier)) // todo
//			return false;
//		if (dst.qualifier & Types::CType::TypeQualifier::CONST)
//			return false;
		if (is_arithmetic(src) && is_arithmetic(dst))
			return true;
		else if (is_pointer(src) && is_pointer(dst))
		{
			if (is_void(*get<Types::Pointer>(src.type).pointed_type) || is_void(*get<Types::Pointer>(dst.type).pointed_type))
				return true;
			return are_compatible(*get<Types::Pointer>(src.type).pointed_type, *get<Types::Pointer>(dst.type).pointed_type);
		}
		return false;
	}

	// standard: 3.2.1.5
	Types::CType	usual_arithmetic_conversion(const Types::CType &t1, const Types::CType &t2)
	{
		using PlainType = Types::PlainType;

		assert(is_arithmetic(t1));
		assert(is_arithmetic(t2));

		const PlainType &pt1 = get<PlainType>(t1.type);
		const PlainType &pt2 = get<PlainType>(t2.type);


		if (pt1.base == PlainType::LONG_DOUBLE || pt2.base == PlainType::LONG_DOUBLE)
			return {PlainType{PlainType::LONG_DOUBLE}};
		else if (pt1.base == PlainType::DOUBLE || pt2.base == PlainType::DOUBLE)
			return {PlainType{PlainType::DOUBLE}};
		else if (pt1.base == PlainType::FLOAT || pt2.base == PlainType::FLOAT)
			return {PlainType{PlainType::FLOAT}};

		else if ((pt1.base == PlainType::LONG_INT && !pt1.is_signed) ||
				(pt2.base == PlainType::LONG_INT && !pt2.is_signed))
			return {PlainType{PlainType::LONG_INT, false}};
		else if (((pt1.base == PlainType::LONG_INT && pt1.is_signed) &&
				(pt2.base == PlainType::INT && !pt2.is_signed)) ||
				((pt2.base == PlainType::LONG_INT && pt2.is_signed) &&
				 (pt1.base == PlainType::INT && !pt1.is_signed)))
			return {PlainType{PlainType::LONG_INT, true}};
		else if (pt1.base == PlainType::LONG_INT || pt2.base == PlainType::LONG_INT)
			return {PlainType{PlainType::LONG_INT, true}};
		else if ((pt1.base == PlainType::INT && !pt1.is_signed) ||
				(pt2.base == PlainType::INT && !pt2.is_signed))
			return {PlainType{PlainType::INT, false}};

		return {PlainType{PlainType::INT, true}};
	}

	Node	scale_offset(Node &n, int scale)
	{
		auto constant_sym = symbolTable.new_constant(scale);
		Node constant{CONSTANT,{},{}, constant_sym, {}, constant_sym->type, constant_sym};
//		DeduceOneType(constant);
		Node ret(Operation::MUL, {n, constant});
		DeduceOneType(ret);
		return ret;
	}

	void	DeduceOneType(Expression::Node &n)
	{
		std::optional<TAC::Operation> tacOper;

		evaluate_constant_expression(n);

		switch (n.operation)
		{
			case CALL:
				;// todo
			{
				Node function_ptr = n.operands.front();
				std::vector<Node> operands(n.operands.begin() + 1, n.operands.end());
				if (!holds_alternative<Types::Pointer>(function_ptr.type.value().type))
				{
					function_ptr = {ADDRESS, {function_ptr}};
					DeduceOneType(function_ptr);
					n.operands.front() = function_ptr;
				}

				assert(holds_alternative<Types::Pointer>(function_ptr.type.value().type));
				auto function_type = std::get<Types::Pointer>(function_ptr.type.value().type).pointed_type->type;
				assert(holds_alternative<Types::FunctionType>(function_type));
				// todo: function pointer
				const auto &f = get<Types::FunctionType>(function_type);


				if (f.parameters.size() == 1 && holds_alternative<Types::PlainType>(f.parameters.front().type) &&
					get<Types::PlainType>(f.parameters.front().type).base == Types::PlainType::VOID)
					assert(operands.empty());
				else if (!f.parameters.empty()) {
					auto it1 = f.parameters.begin();
					auto it2 = operands.begin();
					while (it1 != f.parameters.end() && it2 != operands.end()) {
						assert(is_convertible_to(*it1, it2->type.value()));
						//todo: convert
						n.operand_types.push_back(*it1);
						it1++;
						it2++;
					}
					if (f.variadic)
					{
						for (;it2 != operands.end(); it2++)
							n.operand_types.push_back(it2->type.value());
					}
					assert(it1 == f.parameters.end() && (it2 == operands.end() || f.variadic));
				}
				n.type = *f.return_type;
				break;
			}
			case MEMBER_ACCESS:
			case PTR_MEMBER_ACCESS:
				//todo
				n.type = n.operands.front().type.value();
				break;
			case POST_INC:
			case POST_DEC: {
				assert(is_scalar(n.operands.front().type.value()));
				assert(n.operands.front().type->is_lvalue);

				n.type = n.operands.front().type.value();
				n.type->is_lvalue = false;
				// todo: pointers
				break;
			}
			case PRE_INC:
			case PRE_DEC: {
				assert(is_scalar(n.operands.front().type.value()));
 				assert(n.operands.front().type->is_lvalue);
				// todo: lvalue
				// todo: pointers
				n.type = n.operands.front().type.value();
				n.type->is_lvalue = false;
				break;
			}
			case ADDRESS:
				assert(n.operands.front().type->is_lvalue);
				//todo
				n.type = Types::CType{Types::Pointer{std::shared_ptr<Types::CType>(new Types::CType{n.operands.front().type.value()})},Types::CType::NONE, false};
				break;
			case DEREFERENCE:
				assert(is_pointer(n.operands.front().type.value()));
				//todo
				n.type = *get<Types::Pointer>(n.operands.front().type.value().type).pointed_type;
				n.type->is_lvalue = true;
				break;
			case UPLUS:
			case UMINUS:
				assert(is_arithmetic(n.operands.front().type.value()));
				// todo: promotion
				n.type = n.operands.front().type.value();
			break;
			case SIZEOF_EXPR:
			case SIZEOF_TYPE:
				//todo
				n.type = n.operands.front().type.value();
				break;
			case CAST:
				//todo
				n.type = n.typeArg;
				break;
			case MOD:
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
			case MUL:
			case DIV: {
				assert(is_arithmetic(n.operands[0].type.value()));
				assert(is_arithmetic(n.operands[1].type.value()));
				//todo: conversions
				n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());
				break;
			}
			case ADD: {
				assert(is_scalar(n.operands[0].type.value()));
				assert(is_scalar(n.operands[1].type.value()));
				assert(!is_pointer(n.operands[0].type.value()) || !is_pointer(n.operands[1].type.value()));

				if (is_pointer(n.operands[0].type.value()))
				{
					n.type = n.operands[0].type; // todo
					size_t scale = symbolTable.size_of(*get<Types::Pointer>(n.type.value().type).pointed_type);
					n.operands[1] = scale_offset(n.operands[1], scale);
				}
				else if(is_pointer(n.operands[1].type.value()))
				{
					n.type = n.operands[1].type;
					size_t scale = symbolTable.size_of(*get<Types::Pointer>(n.type.value().type).pointed_type);
					n.operands[0] = scale_offset(n.operands[0], scale);
				}
				else
					n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());
				break;
			}
			case SUB: {
				assert(is_scalar(n.operands[0].type.value()));
				assert(is_scalar(n.operands[1].type.value()));
				assert(is_pointer(n.operands[0].type.value()) || !is_pointer(n.operands[1].type.value()));

				if (is_pointer(n.operands[0].type.value())) {
					if (is_pointer(n.operands[1].type.value())) // pointer difference
						; // todo
					else // pointer decrementation
					{
						n.type = n.operands[0].type; // todo
						size_t scale = symbolTable.size_of(*get<Types::Pointer>(n.type.value().type).pointed_type);
						n.operands[1] = scale_offset(n.operands[1], scale);
					}
				} else // arithmetic substraction
					n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());

				break;
			case BITSHIFT_LEFT:
				// todo integral promotion

				n.type = n.operands[0].type.value();
				break;
			case BITSHIFT_RIGHT:
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
				// todo integral promotion

				n.type = n.operands[0].type.value();
				break;
			case LESSER:
			case GREATER:
			case LESSER_EQUAL:
			case GREATER_EQUAL:
				if (is_arithmetic(n.operands[0].type.value()))
				{
					assert(is_arithmetic(n.operands[0].type.value()));
					assert(is_arithmetic(n.operands[1].type.value()));

				} else
				{
					assert(is_pointer(n.operands[0].type.value()));
					assert(is_pointer(n.operands[1].type.value()));
					// todo pointers compatibility
				}

				n.type = CTYPE_INT;
				break;
			case EQUAL:
			case NOT_EQUAL:
				assert(is_arithmetic(n.operands[0].type.value()));
				assert(is_arithmetic(n.operands[1].type.value()));
				// todo pointers

				n.type = CTYPE_INT;

				break;
			case BITWISE_AND:
			case BITWISE_XOR:
			case BITWISE_OR:
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
				break;
			case LOGICAL_AND:
			case LOGICAL_OR:
				assert(is_scalar(n.operands[0].type.value()));
				assert(is_scalar(n.operands[1].type.value()));
				// todo pointers

				n.type = CTYPE_INT;

				break;
			case ASSIGNMENT:
				//todo: constraints
				assert(n.operands[0].type.value().is_lvalue);
//				assert(type_equivalence(n.operands[0].type.value(), n.operands[1].type.value(), false));
				assert(is_convertible_to(n.operands[0].type.value(), n.operands[1].type.value()));
				assert(!(n.operands[0].type.value().qualifier & Types::CType::CONST));

				n.type = n.operands[0].type.value();
				n.type->is_lvalue = false;

				break;
			case MUL_ASSIGNMENT:
			case DIV_ASSIGNMENT:
			case MOD_ASSIGNMENT:
			case ADD_ASSIGNMENT:
			case SUB_ASSIGNMENT:
			case BITSHIFT_LEFT_ASSIGNMENT:
			case BITSHIFT_RIGHT_ASSIGNMENT:
			case BITWISE_AND_ASSIGNMENT:
			case BITWISE_XOR_ASSIGNMENT:
			case BITWISE_OR_ASSIGNMENT:
				//todo: constraints
				assert(n.operands[0].type.value().is_lvalue);

				n.operand_types = {usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value())};
				n.type = n.operands[0].type.value();
				n.type->is_lvalue = false;

				break;
			case COMMA:
				n.type = n.operands[1].type;
				break;
			}
			default:
				if (!n.operands.empty())
					n.type = n.operands.front().type.value();
			case IDENTIFIER:
				break;
			case CONSTANT:
				break;
			case STRING_LITERAL:
				break;
			case BITWISE_NOT:
				break;
			case LOGICAL_NOT:
				assert(is_scalar(n.operands[0].type.value()));
				// todo pointers

				n.type = CTYPE_INT;
				break;
			case TERNARY:
				break;
		}
	}

	void	EmitOneTAC(Expression::Node &n)
	{
		std::optional<TAC::Operation> tacOper;
		switch (n.operation)
		{
			case CALL:
				;// todo
			{
				Node function_ptr = n.operands.front();
				std::vector<Node> operands(n.operands.begin() + 1, n.operands.end());

				// todo: function pointer
				const auto &f = get<Types::FunctionType>(std::get<Types::Pointer>(function_ptr.type.value().type).pointed_type->type);

				for (size_t i = 0; i <  operands.size(); i++)
				{
//					std::cout << symbolTable.size_of(n.operand_types[i]) << std::endl;
					int operation_size = symbolTable.size_of(n.operand_types[i]);
					int operation_sign = Types::is_signed(n.operand_types[i]);
					TAC::Instruction ins{{}, TAC::PARAM, operands[i].ret_address, {}, operation_size, operation_sign};
					TAC::currentFunction->add_instruction(ins);
				}
				if (!type_equivalence(*f.return_type, {Types::PlainType{{Types::PlainType::VOID}}}))
					n.ret_address = TAC::currentFunction->new_temp(*f.return_type);
				TAC::currentFunction->add_instruction({n.ret_address, TAC::CALL, function_ptr.ret_address});
				break;
			}
			case MEMBER_ACCESS:
			case PTR_MEMBER_ACCESS:
				//todo
				n.type = n.operands.front().type.value();
				break;
			case POST_INC:
			case POST_DEC: {
				auto increment_step = is_pointer(n.type.value()) ? symbolTable.new_constant(symbolTable.size_of(*get<Types::Pointer>(n.type.value().type).pointed_type)) : symbolTable.new_constant(1);
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address, {}, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				if (n.operation == POST_INC)
					TAC::currentFunction->add_instruction(
							{n.operands[0].ret_address, TAC::ADD, n.operands[0].ret_address,
							 increment_step, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				else
					TAC::currentFunction->add_instruction(
							{n.operands[0].ret_address, TAC::SUB, n.operands[0].ret_address,
							 increment_step, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});

				// todo: pointers
				break;
			}
			case PRE_INC:
			case PRE_DEC: {
				// todo: lvalue
				// todo: pointers
				auto increment_step = is_pointer(n.type.value()) ? symbolTable.new_constant(symbolTable.size_of(*get<Types::Pointer>(n.type.value().type).pointed_type)) : symbolTable.new_constant(1);

				n.ret_address = n.operands[0].ret_address;
				if (n.operation == PRE_INC)
					TAC::currentFunction->add_instruction({n.ret_address, TAC::ADD, n.operands[0].ret_address, increment_step, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				else
					TAC::currentFunction->add_instruction({n.ret_address, TAC::SUB, n.operands[0].ret_address, increment_step, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			}
			case ADDRESS:
				//todo
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ADDRESS, n.operands[0].ret_address});
				break;
			case DEREFERENCE:
				//todo

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::DEREFERENCE, n.operands[0].ret_address});
				break;
			case UPLUS:
				n.ret_address = n.operands[0].ret_address;
				break;
			case UMINUS:
				// todo: promotion
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::NEG, n.operands[0].ret_address, {}, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
			break;
			case SIZEOF_EXPR:
			case SIZEOF_TYPE:
				//todo
				break;
			case CAST:
				//todo
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address, {}, symbolTable.size_of(n.typeArg.value()), Types::is_signed(n.typeArg.value())});
				break;
			case MOD:
				if (!tacOper) tacOper = TAC::MOD;
			case MUL:
				if (!tacOper) tacOper = TAC::MUL;
			case DIV: {
				if (!tacOper) tacOper = TAC::DIV;
				//todo: conversions

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			}
			case ADD: {

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction(
						{n.ret_address, TAC::ADD, n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			}
			case SUB: {
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction(
						{n.ret_address, TAC::SUB, n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case BITSHIFT_LEFT:
				// todo integral promotion


				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::SHIFT_LEFT, n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case BITSHIFT_RIGHT:
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::SHIFT_RIGHT, n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case LESSER:
				if (!tacOper) tacOper = TAC::LESSER;
			case GREATER:
				if (!tacOper) tacOper = TAC::GREATER;
			case LESSER_EQUAL:
				if (!tacOper) tacOper = TAC::LESSER_EQUAL;
			case GREATER_EQUAL:
				if (!tacOper) tacOper = TAC::GREATER_EQUAL;
				// todo pointers

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case EQUAL:
				if (!tacOper) tacOper = TAC::EQUAL;
			case NOT_EQUAL:
				if (!tacOper) tacOper = TAC::NOT_EQUAL;
				// todo pointers

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case BITWISE_AND:
				if (!tacOper) tacOper = TAC::BITWISE_AND;
			case BITWISE_XOR:
				if (!tacOper) tacOper = TAC::BITWISE_XOR;
			case BITWISE_OR:
				if (!tacOper) tacOper = TAC::BITWISE_OR;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction(
						{n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				//todo
				break;
			case LOGICAL_AND:
				if (!tacOper) tacOper = TAC::LOGICAL_AND;
			case LOGICAL_OR:
				if (!tacOper) tacOper = TAC::LOGICAL_OR;
				// todo pointers


				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case ASSIGNMENT:
				//todo: constraints
				//todo assert(n.operands[0].type.value().is_lvalue);

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.operands[0].ret_address, TAC::ASSIGN, n.operands[1].ret_address, {}, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address, {}, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case MUL_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::MUL;
			case DIV_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::DIV;
			case MOD_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::MOD;
			case ADD_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::ADD;
			case SUB_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::SUB;
			case BITSHIFT_LEFT_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::SHIFT_LEFT;
			case BITSHIFT_RIGHT_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::SHIFT_RIGHT;
			case BITWISE_AND_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::BITWISE_AND;
			case BITWISE_XOR_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::BITWISE_XOR;
			case BITWISE_OR_ASSIGNMENT:
				if (!tacOper) tacOper = TAC::BITWISE_OR;
				//todo: constraints

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.operands[0].ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address, symbolTable.size_of(n.operand_types[0]), Types::is_signed(n.operand_types[0])});
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address, {}, symbolTable.size_of(n.type.value()), Types::is_signed(n.type.value())});
				break;
			case COMMA:
				n.ret_address = n.operands[1].ret_address;
				break;
			}
			default:
			case IDENTIFIER:
				break;
			case CONSTANT:
				break;
			case STRING_LITERAL:
				break;
			case BITWISE_NOT:
				break;
			case LOGICAL_NOT:
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::LOGICAL_NOT, n.operands[0].ret_address});
				break;
			case TERNARY:
				break;
		}
	}

	void	EmitTac(Expression::Node &n)
	{
		std::stack<Node*>	stack;
		std::set<Node*>		filter;
		stack.push(&n);
		while (!stack.empty())
		{
			Node &current = *stack.top();

			bool is_ready = true;
			for (auto &operand : std::ranges::subrange(current.operands.rbegin(), current.operands.rend())) {
				if (!filter.contains(&operand)) {
					stack.push(&operand);
					filter.insert(&operand);
					is_ready = false;
				}
			}
			if (is_ready)
			{
				EmitOneTAC(current);
				stack.pop();
			}
		}
	}

	void	DeduceType(Expression::Node &n)
	{
		std::stack<Node*> stack;
		stack.push(&n);
		while (!stack.empty())
		{
			Node &current = *stack.top();

			bool is_ready = true;
			for (auto &operand : current.operands) {
				if (!operand.type) {
					stack.push(&operand);
					is_ready = false;
				}
			}
			if (is_ready)
			{
				DeduceOneType(current);
				stack.pop();
			}
		}
	}

	void		evaluate_constant_expression(Expression::Node &n)
	{
		for (auto &op : n.operands)
			if (op.operation != Operation::CONSTANT)
				return;

		uintmax_t l, r;
		if (n.operands.size() > 0)
			l = std::get<uintmax_t>(n.operands[0].constant.value()->value);
		if (n.operands.size() > 1)
			r = std::get<uintmax_t>(n.operands[1].constant.value()->value);
		int value;
		switch (n.operation)
		{
			case UPLUS:
				value = +l;
				break;
			case UMINUS:
				value = -l;
				break;
			case SIZEOF_EXPR:
				value = symbolTable.size_of(n.type.value());
				break;
			case SIZEOF_TYPE:
				value = symbolTable.size_of(n.typeArg.value());
				break;
			case CAST:
				value = l;
				break;
				// todo
			case MOD:
				value = l % r;
				break;
			case MUL:
				value = l * r;
				break;
			case DIV:
				value = l / r;
				break;
			case ADD:
				value = l + r;
				break;
			case SUB:
				value = l - r;
				break;
			case BITSHIFT_LEFT:
				value = l << r;
				break;
			case BITSHIFT_RIGHT:
				value = l >> r;
				break;
			case LESSER:
				value = l < r;
				break;
			case GREATER:
				value = l > r;
			case LESSER_EQUAL:
				value = l <= r;
				break;
			case GREATER_EQUAL:
				value = l >= r;
				break;
			case EQUAL:
				value = l == r;
				break;
			case NOT_EQUAL:
				value = l != r;
				break;
			case BITWISE_AND:
				value = l & r;
			case BITWISE_XOR:
				value = l ^ r;
				break;
			case BITWISE_OR:
				value = l | r;
				break;
			case LOGICAL_AND:
				value = l && r;
				break;
			case LOGICAL_OR:
				value = l || r;
				break;
//			case CONSTANT:
//				switch (n.constant.value()->value.index())
//				{
//					case 0:
//						return get<0>(n.constant.value()->value);
//					case 1:
//						return get<1>(n.constant.value()->value);;
//				}
//			case STRING_LITERAL:
			case BITWISE_NOT:
				value = ~l;
				break;
			case LOGICAL_NOT:
				value = !l;
				break;
			case TERNARY:
				value = l ? r : std::get<uintmax_t>(n.operands[2].constant.value()->value);
				break;
			default:
				return;
		}
		auto *constant = symbolTable.new_constant(value);
		n = {Expression::CONSTANT, {}, std::nullopt, constant, {}, CTYPE_LONG_INT, constant};
	}
}