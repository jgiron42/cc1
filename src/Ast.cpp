#include "Ast.hpp"
#include <iostream>
#include <queue>
#include <functional>

namespace Ast {

	Types::StructOrUnion parse_struct_declaration_list(const StructDeclarationList &l) {
		Types::StructOrUnion ret;
		for (const auto &d: l) {
			auto base = parse_specifier_qualifier_list(d->declarationSpecifierList);
			for (const auto &e: d->structDeclaratorList) {
				Types::StructOrUnion::Entry *entry = &ret.members.emplace_back();
				if (e.bitfield)
					entry->bitfield = evaluate_constant_expression(e.bitfield.value());
				if (e.declarator) {
					auto [name, type, _] = parse_declarator(base, &e.declarator.value());
					if (ret.member_map.contains(name.value()))
						std::cerr << "duplicated member" << std::endl;//todo error duplicated
					entry->name = name.value();
					entry->type = std::shared_ptr<Types::CType>(new Types::CType(type));
					ret.member_map.insert({entry->name, entry});
				}
			}
		}
		return ret;
	}

	Types::TagName parse_struct_or_union_specifier(const StructOrUnionSpecifier &s) {
		std::string name;
		if (s.tag)
			name = s.tag.value();
		else {
			static int uid = 0;
			name = "@anonymous_struct_or_union_" + std::to_string(uid++);
		}
		Types::Tag::Type tag_type =
				s.type == StructOrUnion::STRUCT ? Types::Tag::STRUCT : Types::Tag::UNION;
		Types::Tag *existing = symbolTable.retrieve_tag(name);
		if (!existing)
			symbolTable.declare_tag(name, tag_type);
		else if ((existing->type == Types::Tag::UNION) ^ (s.type == StructOrUnion::UNION)) {
			// todo error redeclaration as a different operation
		} else if (s.declaration && existing->declaration.index()) {
			// todo error duplicated definition
		}
		if (s.declaration) {
			Types::StructOrUnion sou = parse_struct_declaration_list(s.declaration.value());
			symbolTable.assign_tag(name, sou);
		}
		return {tag_type, name};
	}

	SymbolTable::Ordinary parse_declaration_specifier_list(const DeclarationSpecifierList &declarationSpecifierList) {
		SymbolTable::Ordinary ordinary;

		int storage_specifiers_count = std::count_if(declarationSpecifierList.begin(), declarationSpecifierList.end(),
													 [](const DeclarationSpecifier &declarationSpecifier) -> bool {
														 return holds_alternative<StorageSpecifier>(
																 declarationSpecifier);
													 });
		if (storage_specifiers_count > 1); // todo error
		else if (storage_specifiers_count == 0)
			ordinary.storage = SymbolTable::Ordinary::UNDEFINED; // todo, depend on the scope
		else
			ordinary.storage = std::get<StorageSpecifier>(
					*std::find_if(declarationSpecifierList.begin(), declarationSpecifierList.end(),
								  [](const DeclarationSpecifier &declarationSpecifier) -> bool {
									  return holds_alternative<StorageSpecifier>(declarationSpecifier);
								  }));

		auto type_specifiers = declarationSpecifierList
							   | std::views::filter([](const DeclarationSpecifier &declarationSpecifier) -> bool {
			return holds_alternative<TypeSpecifier>(declarationSpecifier);
		})
							   | std::views::transform(
				[](const DeclarationSpecifier &declarationSpecifier) -> const TypeSpecifier & {
					return std::get<TypeSpecifier>(declarationSpecifier);
				});
		ordinary.type = parse_specifier_qualifier_list(declarationSpecifierList);

		for (auto &ds : declarationSpecifierList)
			if (holds_alternative<TypeQualifier>(ds))
				ordinary.type.qualifier |= (Types::CType::TypeQualifier)std::get<TypeQualifier>(ds);
		return ordinary;
	}

	int evaluate_constant_expression(const ConstantExpression &ce) {
		return 42;
		std::stack<const ConstantExpression *> s;
		std::stack<int> values;
		s.push(&ce);

		while (!s.empty()) {
//		switch (s.top()->operation)
//		{
//			case Expression::
//		}
		}
		return values.top();
	}

/*
 * the goal of the following function is to reverse the scoping of the c parsing to reflect the actual operation nesting.
 * for example: int (*(a[4]))(int)
 * parsing:
 *	int (X)(int)
 *	     └── *X
 *		      └──X[4]
 *			     └── a
 *
 * actual nesting:
 * a is an array of pointer to function of prototype int f(int)
 * name: a
 * operation: X[4]
 *       └── *X
 *            └──┐
 *           int f(int)
 * return: (name, type, parameters)
 */
	std::tuple<std::optional<std::string>, Types::CType, std::optional<std::list<std::string>>>
	parse_declarator(Types::CType base, const Declarator *declarator) {
		std::optional<std::list<std::string>> parameters = std::nullopt;
		begin_declarator:
		for (auto &p: declarator->pointer)
			base = {
					Types::Pointer{.pointed_type = std::shared_ptr<Types::CType>(
							new Types::CType(base))},
					p.value_or(TypeQualifier::NONE)
			};

		const auto *dd = &declarator->directDeclarator;
		while (1) {
			if (holds_alternative<std::string>(*dd))
				return {get<std::string>(*dd), base, parameters};
			else if (holds_alternative<std::monostate>(*dd))
				return {std::nullopt, base, parameters};
			else if (holds_alternative<NestedDeclarator>(*dd)) {
				declarator = get<NestedDeclarator>(*dd).get();
				goto begin_declarator;
			} else if (holds_alternative<ArrayDeclarator>(*dd)) {
				base = {
						Types::Array{
								.value_type{std::shared_ptr<Types::CType>(new Types::CType(base))}
						},
						TypeQualifier::NONE
				};
				if (get<ArrayDeclarator>(*dd).constantExpression.has_value())
					get<Types::Array>(base.type).size = evaluate_constant_expression(
							get<ArrayDeclarator>(*dd).constantExpression.value());
				dd = get<ArrayDeclarator>(*dd).directDeclarator.get();
			} else if (holds_alternative<FunctionDeclarator>(*dd)) {
				auto &functionDeclarator = get<FunctionDeclarator>(*dd);

				Types::FunctionType functionType;

				functionType.return_type = std::shared_ptr<Types::CType>(new Types::CType(base));
				if (parameters)
					parameters->clear();
				else
					parameters.emplace();
				if (holds_alternative<ParameterTypeList>(functionDeclarator.parameter_type_list_or_identifier_list)) {
					auto &parameterTypeList = get<ParameterTypeList>(functionDeclarator.parameter_type_list_or_identifier_list);
					for (auto &p : parameterTypeList.parameterList) {

						if (p.declarator)
						{
							SymbolTable::Ordinary base = parse_declaration_specifier_list(p.declarationSpecifierList);
							auto [name, type, _] = parse_declarator(base.type, p.declarator.value().get());
							functionType.parameters.emplace_back(type);
							if (name) {
								parameters->push_back(name.value());
							}
						}
						else
							functionType.parameters.emplace_back(parse_type_name({p.declarationSpecifierList, std::nullopt}));
					}
					functionType.variadic = parameterTypeList.ellipsis;
				}
				else if (holds_alternative<IdentifierList>(functionDeclarator.parameter_type_list_or_identifier_list))
					parameters = get<IdentifierList>(functionDeclarator.parameter_type_list_or_identifier_list);

				base = {
						functionType,
						TypeQualifier::NONE
				};
				dd = functionDeclarator.directDeclarator.get();
			}
		}
	}

	bool is_const_expression(const ConstantExpression &ce) {
		std::queue<const ConstantExpression *> to_check;
		to_check.push(&ce);
		while (!to_check.empty()) {
			const ConstantExpression *current = to_check.front();
			to_check.pop();
			static const std::set forbiden_operators = {
					Expression::ASSIGNMENT,
					Expression::ADD_ASSIGNMENT,
					Expression::SUB_ASSIGNMENT,
					Expression::MUL_ASSIGNMENT,
					Expression::MOD_ASSIGNMENT,
					Expression::BITSHIFT_LEFT_ASSIGNMENT,
					Expression::BITSHIFT_RIGHT_ASSIGNMENT,
					Expression::BITWISE_AND_ASSIGNMENT,
					Expression::BITWISE_OR_ASSIGNMENT,
					Expression::BITWISE_XOR_ASSIGNMENT,
					Expression::POST_DEC,
					Expression::POST_INC,
					Expression::PRE_DEC,
					Expression::PRE_INC,
					Expression::CALL,
					Expression::COMMA,
			};
			if (forbiden_operators.contains(current->operation))
				return false;
			if (current->operation != Expression::SIZEOF_EXPR)
				for (const auto &sce: current->operands)
					to_check.push(&sce);
		}
		return true;
	}

	Types::CType	parse_type_name(const TypeName &tn)
	{
		SymbolTable::Ordinary base = parse_declaration_specifier_list(tn.declarationSpecifierList);
		if (tn.abstractDeclarator)
			return get<1>(parse_declarator(base.type, &tn.abstractDeclarator.value()));
		else
			return base.type;
	}

	void init(const std::string &name, const Types::CType &type, const Ast::Initializer &init)
	{
		switch (type.type.index())
		{
			case 0: // PlainType
			{
				switch (get<0>(type.type).base)
				{
					case Types::PlainType::VOID:
					case Types::PlainType::CHAR:
					case Types::PlainType::SHORT_INT:
					case Types::PlainType::INT:
					case Types::PlainType::LONG_INT:
					case Types::PlainType::FLOAT:
					case Types::PlainType::DOUBLE:
					case Types::PlainType::LONG_DOUBLE:
					throw std::runtime_error("unsupported");
				}
			}
				break;
			case 1: // TagName
				throw std::runtime_error("Tags are unsupported");
				//todo: tags
				break;
			case 2: // Pointer
				throw std::runtime_error("unsupported");
			case 3: // FunctionType
				throw std::runtime_error("Can't assign a function"); //todo: better
			case 4: // Array
				throw std::runtime_error("unsupported");
			case 5: // Typename
				throw std::runtime_error("unsupported");
			default:
				throw std::runtime_error("shouldn't be reached");
		}
	}

	void Declare(Declaration &declaration) {
		SymbolTable::Ordinary base = parse_declaration_specifier_list(declaration.specifiers);
		if (!declaration.init)
			return;
		for (const auto &dcl: declaration.init.value()) {
			auto [name, type, _] = parse_declarator(base.type, &dcl.declarator);
			if (name) {
				if (!symbolTable.insert_ordinary(name.value(), {base.storage, type, name.value()}))
					std::cout << "duplicate function: " << name.value() << std::endl;
			}
			if (dcl.initializer)
				init(name.value(), type, dcl.initializer.value());
			// todo duplicate
			// todo: init;
		}
	}
	void Declare(std::list<Declaration> &declaration) {
		for (auto &d : declaration)
			Declare(d);
	}

	std::string DeclareFunction(const FunctionDefinitionDeclaration &declaration) {
		Declaration tmp = {
				declaration.declarationSpecifierList.value_or(
						Ast::DeclarationSpecifierList {Ast::DeclarationSpecifier{Ast::TypeSpecifier{Ast::TypeSpecifier::INT}}}
						),
				InitDeclaratorList{InitDeclarator{declaration.declarator}}
		};
		SymbolTable::Function f;
		SymbolTable::Ordinary base = parse_declaration_specifier_list(tmp.specifiers);
		auto [name, type, param_names] = parse_declarator(base.type, &declaration.declarator);
		// todo: check if type is a function
		// todo: check if identifier_list and
		assert(param_names);
		assert(std::set<std::string>(param_names->begin(), param_names->end()).size() == param_names->size()); // duplicates params todo
		if (!param_names->empty() && get<Types::FunctionType>(type.type).parameters.empty()) // identifier list
		{
			for (auto &n : param_names.value())
			{
				auto *s  = symbolTable.retrieve_ordinary(n);
				assert(s); // todo
				get<Types::FunctionType>(type.type).parameters.emplace_back(s->type);
			}
		}
		else
		{
			assert(param_names->size() == get<Types::FunctionType>(type.type).parameters.size());
			auto it_names = param_names->begin();
			auto it_params = get<Types::FunctionType>(type.type).parameters.begin();
			for (; it_names != param_names->end(); it_names++, it_params++)
			{
				if (!symbolTable.insert_ordinary(*it_names, {SymbolTable::Ordinary::UNDEFINED, *it_params, *it_names}))
					std::cout << "duplicate param: " << *it_names << std::endl; // todo
			}
		}
		assert(name);
		if (!symbolTable.insert_function(name.value(), {base.storage, type, name.value()}))
			std::cout << "duplicate function: " << name.value() << std::endl;
		f.type = type;
		f.name = name.value();
		for (const auto &p : *param_names)
			f.params.push_back(symbolTable.retrieve_ordinary(p));
		std::cout << "enter " <<  f.name << std::endl;
		symbolTable.enter_function(f);
		// todo duplicate
		return name.value();
	}

}