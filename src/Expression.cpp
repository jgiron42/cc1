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
					return false;
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

	void	DeduceOneType(Expression::Node &n)
	{
		std::optional<TAC::Operation> tacOper;
		switch (n.operation)
		{
			case CALL:
				;// todo
			{
				Node function = n.operands.front();
				std::vector<Node> operands(n.operands.begin() + 1, n.operands.end());

				assert(holds_alternative<Types::FunctionType>(function.type.value().type));
				// todo: function pointer
				const auto &f = get<Types::FunctionType>(function.type.value().type);


				if (f.parameters.size() == 1 && holds_alternative<Types::PlainType>(f.parameters.front().type) &&
					get<Types::PlainType>(f.parameters.front().type).base == Types::PlainType::VOID)
					assert(operands.empty());
				else if (!f.parameters.empty()) {
					auto it1 = f.parameters.begin();
					auto it2 = operands.begin();
					while (it1 != f.parameters.end() && it2 != operands.end()) {
						//todo: convert
						it1++;
						it2++;
					}
					assert(it1 == f.parameters.end() && (it2 == operands.end() || f.variadic));
				}
				for (const auto &op : operands)
					TAC::currentFunction->add_instruction({{}, TAC::PARAM, op.ret_address});
				n.ret_address = TAC::currentFunction->new_temp(*f.return_type);
				TAC::currentFunction->add_instruction({n.ret_address, TAC::CALL, function.ret_address});
				n.type = *f.return_type;
				break;
			}
			case MEMBER_ACCESS:
			case PTR_MEMBER_ACCESS:
				//todo
				n.type = n.operands.front().type.value();
				break;
			case POST_INC:
			case POST_DEC:
				assert(is_arithmetic(n.operands.front().type.value()));
				assert(n.operands.front().type->is_lvalue);

				n.type = n.operands.front().type.value();
				n.type->is_lvalue = false;
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address});
				if (n.operation == POST_INC)
					TAC::currentFunction->add_instruction({n.operands[0].ret_address, TAC::ADD, n.operands[0].ret_address, symbolTable.new_constant(1)});
				else
					TAC::currentFunction->add_instruction({n.operands[0].ret_address, TAC::SUB, n.operands[0].ret_address, symbolTable.new_constant(1)});

				// todo: pointers
				break;
			case PRE_INC:
			case PRE_DEC: {
				assert(is_arithmetic(n.operands.front().type.value()));
				assert(n.operands.front().type->is_lvalue);
				// todo: lvalue
				// todo: pointers
				n.type = n.operands.front().type.value();
				n.type->is_lvalue = false;

				n.ret_address = n.operands[0].ret_address;
				if (n.operation == PRE_INC)
					TAC::currentFunction->add_instruction({n.ret_address, TAC::ADD, n.operands[0].ret_address, symbolTable.new_constant(1)});
				else
					TAC::currentFunction->add_instruction({n.ret_address, TAC::SUB, n.operands[0].ret_address, symbolTable.new_constant(1)});

				break;
			}
			case ADDRESS:
				assert(n.operands.front().type->is_lvalue);
				//todo
				n.type = Types::CType{Types::Pointer{std::shared_ptr<Types::CType>(new Types::CType{n.operands.front().type.value()})},Types::CType::NONE, false};
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ADDRESS, n.operands[0].ret_address});
				break;
			case DEREFERENCE:
				assert(is_pointer(n.operands.front().type.value()));
				//todo
				n.type = *get<Types::Pointer>(n.operands.front().type.value().type).pointed_type;
				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::DEREFERENCE, n.operands[0].ret_address});
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
				n.type = n.operands.front().type.value();
				break;
			case MOD:
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
				if (!tacOper) tacOper = TAC::MOD;
			case MUL:
				if (!tacOper) tacOper = TAC::MUL;
			case DIV: {
				assert(is_arithmetic(n.operands[0].type.value()));
				assert(is_arithmetic(n.operands[1].type.value()));
				if (!tacOper) tacOper = TAC::DIV;
				//todo: conversions
				n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			}
			case ADD: {
				assert(is_scalar(n.operands[0].type.value()));
				assert(is_scalar(n.operands[1].type.value()));
				assert(!is_pointer(n.operands[0].type.value()) || !is_pointer(n.operands[1].type.value()));

				if (is_pointer(n.operands[0].type.value()))
					n.type = n.operands[0].type;//todo
				else if(is_pointer(n.operands[1].type.value()))
					n.type = n.operands[1].type;
				else
					n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction(
						{n.ret_address, TAC::ADD, n.operands[0].ret_address, n.operands[1].ret_address});
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
						; // todo
				} else // arithmetic substraction
					n.type = usual_arithmetic_conversion(n.operands[0].type.value(), n.operands[1].type.value());

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction(
						{n.ret_address, TAC::SUB, n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case BITSHIFT_LEFT:
				// todo integral promotion

				n.type = n.operands[0].type.value();

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::SHIFT_LEFT, n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case BITSHIFT_RIGHT:
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
				// todo integral promotion

				n.type = n.operands[0].type.value();

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, TAC::SHIFT_RIGHT, n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case LESSER:
				if (!tacOper) tacOper = TAC::LESSER;
			case GREATER:
				if (!tacOper) tacOper = TAC::GREATER;
			case LESSER_EQUAL:
				if (!tacOper) tacOper = TAC::LESSER_EQUAL;
			case GREATER_EQUAL:
				if (!tacOper) tacOper = TAC::GREATER_EQUAL;
				assert(is_arithmetic(n.operands[0].type.value()));
				assert(is_arithmetic(n.operands[1].type.value()));
				// todo pointers

				n.type = CTYPE_INT;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case EQUAL:
				if (!tacOper) tacOper = TAC::EQUAL;
			case NOT_EQUAL:
				if (!tacOper) tacOper = TAC::NOT_EQUAL;
				assert(is_arithmetic(n.operands[0].type.value()));
				assert(is_arithmetic(n.operands[1].type.value()));
				// todo pointers

				n.type = CTYPE_INT;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case BITWISE_AND:
				if (!tacOper) tacOper = TAC::BITWISE_AND;
			case BITWISE_XOR:
				if (!tacOper) tacOper = TAC::BITWISE_XOR;
			case BITWISE_OR:
				if (!tacOper) tacOper = TAC::BITWISE_OR;
				assert(is_integral(n.operands[0].type.value()));
				assert(is_integral(n.operands[1].type.value()));
				break;
			case LOGICAL_AND:
				if (!tacOper) tacOper = TAC::LOGICAL_AND;
			case LOGICAL_OR:
				if (!tacOper) tacOper = TAC::LOGICAL_OR;
				assert(is_scalar(n.operands[0].type.value()));
				assert(is_scalar(n.operands[1].type.value()));
				// todo pointers

				n.type = CTYPE_INT;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				break;
			case ASSIGNMENT:
				//todo: constraints
				//todo assert(n.operands[0].type.value().is_lvalue);

				n.type = n.operands[0].type.value();
				n.type->is_lvalue = false;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.operands[0].ret_address, TAC::ASSIGN, n.operands[1].ret_address});
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address});
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
				assert(n.operands[0].type.value().is_lvalue);

				n.type = n.operands[0].type.value();
				n.type->is_lvalue = false;

				n.ret_address = TAC::currentFunction->new_temp(n.type.value());
				TAC::currentFunction->add_instruction({n.operands[0].ret_address, tacOper.value(), n.operands[0].ret_address, n.operands[1].ret_address});
				TAC::currentFunction->add_instruction({n.ret_address, TAC::ASSIGN, n.operands[0].ret_address});
				break;
			case COMMA:
				n.type = n.operands[1].type;
				n.ret_address = n.operands[1].ret_address;
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
				break;
			case TERNARY:
				break;
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
}