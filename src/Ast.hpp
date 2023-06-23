#ifndef CC1_POC_AST_HPP
#define CC1_POC_AST_HPP
#include <memory>
#include <variant>
#include <optional>
#include <array>
#include <list>
#include <string>
#include <ranges>
#include "SymbolTable.hpp"
#include "Expression.hpp"

namespace Ast {

	typedef std::vector<Expression::Node> ExpressionList;

	typedef Expression::Node ConstantExpression;
	struct StructDeclaration;
	typedef std::list<std::shared_ptr<StructDeclaration>> StructDeclarationList;
	enum StructOrUnion {
		STRUCT,
		UNION
	};
	struct StructOrUnionSpecifier {
		StructOrUnion type;
		std::optional<std::string> tag;
		std::optional<StructDeclarationList> declaration;
	};

	struct Enumerator {
		std::string name;
		std::optional<int> value;
	};
	typedef std::list<Enumerator> EnumeratorList;
	struct EnumSpecifier {
		std::optional<std::string> tag;
		std::optional<EnumeratorList> declaration;
	};

	struct TypeSpecifier {
		enum Type {
			INT = 0, // default
			STRUCT_OR_UNION,
			ENUM,
			TYPE_NAME,
			VOID,
			CHAR,
			FLOAT,
			SHORT,
			LONG,
			DOUBLE,
			SIGNED,
			UNSIGNED,
			Size
		};
		Type type;
		std::variant<std::monostate, StructOrUnionSpecifier, EnumSpecifier, std::string> value;
	};
	typedef Types::CType::TypeQualifier TypeQualifier;
	typedef SymbolTable::Ordinary::Storage StorageSpecifier;
	typedef std::variant<StorageSpecifier, TypeSpecifier, TypeQualifier> DeclarationSpecifier;
	typedef std::list<DeclarationSpecifier> DeclarationSpecifierList;
	typedef std::optional<DeclarationSpecifierList> DeclarationSpecifierListOpt;

	// each element in the list represent a pointer with an optional qualifier
	typedef std::list<std::optional<TypeQualifier>> Pointer;

	struct Declarator;
	typedef std::shared_ptr<Declarator> NestedDeclarator;

	struct FunctionDeclarator;
	struct ArrayDeclarator {
		std::shared_ptr<std::variant<
		std::monostate,
				std::string,
				NestedDeclarator,
				ArrayDeclarator,
				FunctionDeclarator
		> > directDeclarator; // we cant forward declare that
		std::optional<ConstantExpression> constantExpression;
	};

	struct ParameterDeclaration {
		DeclarationSpecifierList		declarationSpecifierList;
		std::optional<NestedDeclarator>	declarator;
	};
	typedef std::list<ParameterDeclaration> ParameterList;

	struct ParameterTypeList {
		ParameterList parameterList;
		bool ellipsis;
	};

	typedef std::list<std::string> IdentifierList;

	struct FunctionDeclarator {
		std::shared_ptr<std::variant<
		std::monostate,
				std::string,
				NestedDeclarator,
				ArrayDeclarator,
				FunctionDeclarator
		> > directDeclarator; // we cant forward declare that
		std::variant<
				std::monostate,
				ParameterTypeList,
				IdentifierList
		>		parameter_type_list_or_identifier_list;
	};

	typedef std::variant<
		std::monostate,
		std::string,
		NestedDeclarator,
		ArrayDeclarator,
		FunctionDeclarator
	> DirectDeclarator;
	typedef std::shared_ptr<DirectDeclarator> DirectDeclaratorPointer;

	struct Declarator {
		Pointer				pointer;
		DirectDeclarator	directDeclarator;
	};

	struct TypeName {
		DeclarationSpecifierList declarationSpecifierList;
		std::optional<Declarator> abstractDeclarator;
	};

	struct StructDeclarator {
		std::optional<Declarator>			declarator;
		std::optional<ConstantExpression>	bitfield;
	};
	typedef std::list<StructDeclarator> StructDeclaratorList;

	struct StructDeclaration {
		DeclarationSpecifierList	declarationSpecifierList;
		StructDeclaratorList	structDeclaratorList;
	};

	struct Initializer {
		std::variant<
				Expression::Node,
				std::list<Initializer>
					> value;
	};

	typedef std::list<Initializer> InitializerList;

	struct InitDeclarator {
		Declarator declarator;
		std::optional<Initializer> initializer;
	};

	typedef std::list<InitDeclarator> InitDeclaratorList;

	struct Declaration {
		DeclarationSpecifierList			specifiers;
		std::optional<InitDeclaratorList>	init;
	};
	typedef std::list<Declaration> DeclarationList;

	struct Statement;
	typedef std::shared_ptr<Statement> StatementPtr;
	typedef std::list<Statement> StatementList;

	struct LabeledStatement {
		std::variant <
		        std::monostate, // default
				ConstantExpression, // case
				std::string // label
		        > name;
		StatementPtr statement;
	};

	struct CompoundStatement {
		int								scope;
		std::optional<DeclarationList>	declarationList;
		std::optional<StatementList>	statementList;
	};

	struct ExpressionStatement {
		std::optional<Expression::Node> expression;
	};

	struct SelectionStatement {
		enum Type {
			IF,
			SWITCH
		} type;
		Expression::Node expression;
		StatementPtr statement;
		std::optional<StatementPtr> else_statement;
	};

	struct IterationStatement {
		enum Type {
			WHILE,
			DO_WHILE,
			FOR
		} type;
		std::optional<Expression::Node>	condition;
		StatementPtr				statement;

		std::optional<Expression::Node>	init;
		std::optional<Expression::Node>	update;
	};

	struct JumpStatement {
		enum Type {
			GOTO,
			CONTINUE,
			BREAK,
			RETURN
		} type;
		std::variant<
			std::monostate,
			std::string,
			Expression::Node
			> value;
	};

	struct Statement {
		std::variant<
				LabeledStatement,
				CompoundStatement,
				ExpressionStatement,
				SelectionStatement,
				IterationStatement,
				JumpStatement
		        > value;
	};

	struct FunctionDefinitionDeclaration {
		std::optional<DeclarationSpecifierList> declarationSpecifierList;
		Declarator								declarator;
		int										block_id;
	};

	struct FunctionDefinition {
		FunctionDefinitionDeclaration	declaration;
		std::optional<DeclarationList>	declarationList;
		CompoundStatement				statement;
	};

	typedef std::variant<FunctionDefinition, Declaration> ExternalDeclaration;

	typedef std::list<ExternalDeclaration> TranslationUnit;

	Types::StructOrUnion parse_struct_declaration_list(const StructDeclarationList & l);

	Types::TagName parse_struct_or_union_specifier(const StructOrUnionSpecifier & s);

	template <std::ranges::range R>
	Types::CType	parse_specifier_qualifier_list(R &&range)
	{
		Types::CType ret;

		for (auto &ds : range)
			if (holds_alternative<TypeQualifier>(ds))
				ret.qualifier |= (Types::CType::TypeQualifier)std::get<TypeQualifier>(ds);

		auto type_specifier_range = range
									| std::views::filter([](const DeclarationSpecifier &declarationSpecifier) -> bool {return holds_alternative<TypeSpecifier>(declarationSpecifier);})
									| std::views::transform([](const DeclarationSpecifier &declarationSpecifier) -> const TypeSpecifier& {return std::get<TypeSpecifier>(declarationSpecifier);});
		std::array<int, TypeSpecifier::Type::Size> array = {};
		for (auto &e : type_specifier_range)
			array[e.type]++;

		// here are a set of rules on the presence of the different type specifiers
		if (array[TypeSpecifier::LONG] + array[TypeSpecifier::SHORT] > 1) // a type cannot be short and long
		{}//todo error
		else if (array[TypeSpecifier::SIGNED] + array[TypeSpecifier::UNSIGNED] > 1) // a type cannot be signed and unsigned
		{}//todo error
		else if (array[TypeSpecifier::INT] + array[TypeSpecifier::TYPE_NAME] +
				 array[TypeSpecifier::STRUCT_OR_UNION] + array[TypeSpecifier::ENUM] +
				 array[TypeSpecifier::VOID] + array[TypeSpecifier::CHAR] +
				 array[TypeSpecifier::FLOAT] + array[TypeSpecifier::DOUBLE]
				 > 1) // there must be only one base type specified
		{}//todo error
		else if (array[TypeSpecifier::LONG] + array[TypeSpecifier::INT] +
				 array[TypeSpecifier::DOUBLE] < 2) // only an int or a double can be long
		{}//todo error
		else if (array[TypeSpecifier::SHORT] + array[TypeSpecifier::INT] < 2) // only an int can be short
		{}//todo error
		else if (array[TypeSpecifier::SIGNED] + array[TypeSpecifier::UNSIGNED] +
				 array[TypeSpecifier::INT] + array[TypeSpecifier::CHAR]) // only an int or a char can be signed or unsigned
		{}//todo error

		if (array[TypeSpecifier::TYPE_NAME])
		{
			const auto &s = std::get<std::string>((*std::find_if(type_specifier_range.begin(), type_specifier_range.end(), [](const TypeSpecifier &typeSpecifier) -> bool {return typeSpecifier.type == TypeSpecifier::TYPE_NAME;})).value);
			ret = symbolTable.retrieve_ordinary(s)->type;
		}
		else if (array[TypeSpecifier::STRUCT_OR_UNION])
		{
			const auto &structOrUnionSpecifier = std::get<StructOrUnionSpecifier>((*std::find_if(type_specifier_range.begin(), type_specifier_range.end(), [](const TypeSpecifier &typeSpecifier) -> bool {return typeSpecifier.type == TypeSpecifier::STRUCT_OR_UNION;})).value);
			ret.type = parse_struct_or_union_specifier(structOrUnionSpecifier);
		}
		else if (array[TypeSpecifier::ENUM])
		{
			const auto &enumSpecifier = std::get<EnumSpecifier>((*std::find_if(type_specifier_range.begin(), type_specifier_range.end(), [](const TypeSpecifier &typeSpecifier) -> bool {return typeSpecifier.type == TypeSpecifier::ENUM;})).value);
			//todo ret.operation = parse_enum_specifier(enumSpecifier);
		}
		else // plain
		{
			Types::PlainType plain;
			plain.is_signed = true;
			if (array[TypeSpecifier::VOID])
				plain.base = Types::PlainType::VOID;
			if (array[TypeSpecifier::CHAR]) {
				plain.base = Types::PlainType::CHAR;
				plain.is_signed = false; // chars are defaulted to unsigned
			} else if (array[TypeSpecifier::INT]) {
				if (array[TypeSpecifier::SHORT])
					plain.base = Types::PlainType::SHORT_INT;
				else if (array[TypeSpecifier::LONG])
					plain.base = Types::PlainType::LONG_INT;
				else
					plain.base = Types::PlainType::INT;
			} else if (array[TypeSpecifier::FLOAT])
				plain.base = Types::PlainType::FLOAT;
			else if (array[TypeSpecifier::DOUBLE]) {
				if (array[TypeSpecifier::LONG])
					plain.base = Types::PlainType::LONG_DOUBLE;
				else
					plain.base = Types::PlainType::DOUBLE;
			}

			if (array[TypeSpecifier::UNSIGNED])
				plain.is_signed = false;
			else if (array[TypeSpecifier::SIGNED])
				plain.is_signed = true;
			ret.type = plain;
		}
		return ret;
	}

	SymbolTable::Ordinary	parse_declaration_specifier_list(const DeclarationSpecifierList &declarationSpecifierList);

	bool					is_const_expression(const ConstantExpression &e);

	int						evaluate_constant_expression(const ConstantExpression &ce);

	std::tuple<std::optional<std::string>, Types::CType, std::optional<std::list<std::string>>>	parse_declarator(Types::CType base, const Declarator *declarator);

	Types::CType			parse_type_name(const TypeName &);

	void					Declare(Declaration &declaration);
	void					Declare(std::list<Declaration> &declaration);
	std::string				DeclareFunction(const FunctionDefinitionDeclaration &function);
}

extern Ast::TranslationUnit tree;

#endif
