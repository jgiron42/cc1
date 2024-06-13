

%{
#include <string>
#include "SymbolTable.hpp"
#include "Ast.hpp"
#include "CommandLine.hpp"
int yyerror(char *s);
int yyerror(const std::string &s);

extern Ast::DeclarationSpecifierList current_declaration_specifier_list;

%}

%token <std::string> IDENTIFIER
%token <const SymbolTable::Constant*> CONSTANT
%token <const SymbolTable::Constant*> FLOAT_CONSTANT
%token <const SymbolTable::Constant*> STRING_LITERAL
%token SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN
%token <std::string> TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%type <Ast::TypeSpecifier> type_specifier
%type <Ast::TypeQualifier> type_qualifier
%type <Ast::StorageSpecifier> storage_class_specifier
%type <Ast::DeclarationSpecifier> declaration_specifier
%type <Ast::DeclarationSpecifierList> declaration_specifier_list
%type <Ast::DeclarationSpecifierList> specifier_qualifier_list
%type <Ast::DeclarationSpecifierListOpt> function_declaration_specifier_list
%type <Ast::StructOrUnion> struct_or_union
%type <Ast::StructDeclaration> struct_declaration
%type <Ast::StructDeclarationList> struct_declaration_list
%type <Ast::StructOrUnionSpecifier> struct_or_union_specifier
%type <Ast::StructDeclarator> struct_declarator
%type <Ast::StructDeclaratorList> struct_declarator_list
%type <Ast::Enumerator> enumerator
%type <Ast::EnumeratorList> enumerator_list
%type <Ast::EnumSpecifier> enum_specifier
%type <Ast::InitDeclaratorList> init_declarator_list
%type <Ast::InitDeclarator> init_declarator
%type <Ast::Pointer> pointer
%type <Ast::IdentifierList> identifier_list
%type <Ast::ParameterList> parameter_list
%type <Ast::ParameterDeclaration> parameter_declaration
%type <Ast::ParameterTypeList> parameter_type_list
%type <Ast::TypeQualifier> type_qualifier_list

%type <Ast::FunctionDeclarator> function_declarator
%type <Ast::ArrayDeclarator> array_declarator
%type <Ast::NestedDeclarator> nested_declarator
%type <Ast::DirectDeclarator> direct_declarator
%type <Ast::DirectDeclaratorPointer> direct_declarator_pointer
%type <Ast::Declarator> declarator

%type <Ast::FunctionDeclarator> function_abstract_declarator
%type <Ast::ArrayDeclarator> array_abstract_declarator
%type <Ast::NestedDeclarator> nested_abstract_declarator
%type <Ast::DirectDeclarator> direct_abstract_declarator
%type <Ast::DirectDeclaratorPointer> direct_abstract_declarator_pointer
%type <Ast::Declarator> abstract_declarator

%type <Ast::ConstantExpression> constant_expression
%type <Ast::Initializer> initializer
%type <Ast::InitializerList> initializer_list
%type <Ast::Declaration> declaration
%type <Ast::DeclarationList> declaration_list declaration_list_declared

%type <Ast::TypeName> type_name
%type <Expression::Node> primary_expression
			postfix_expression
			unary_expression
			cast_expression
			multiplicative_expression
			additive_expression
			shift_expression
			relational_expression
			equality_expression
			and_expression
			exclusive_or_expression
			inclusive_or_expression
			logical_and_expression
			logical_or_expression
			conditional_expression
			assignment_expression
			comma_expression
			expression
			constant_expression
%type <Expression::Operation> unary_operator assignment_operator
%type <Ast::ExpressionList> argument_expression_list

%type <Ast::Statement> statement
%type <Ast::StatementPtr> statement_ptr
%type <Ast::StatementList> statement_list
%type <Ast::CompoundStatement> compound_statement
%type <Ast::StatementPtr> else
%type <Ast::LabeledStatement> labeled_statement
%type <Ast::ExpressionStatement> expression_statement
%type <Ast::SelectionStatement> selection_statement
%type <Ast::IterationStatement> iteration_statement
%type <Ast::JumpStatement> jump_statement
%type <Ast::TranslationUnit> translation_unit
%type <Ast::ExternalDeclaration> external_declaration
%type <Ast::FunctionDefinition> function_definition
%type <Ast::FunctionDefinitionDeclaration> function_definition_declaration

%type <int> enter_block enter_function

%start translation_unit

%use_cpp_lex "scanner.yy.hpp"
%variant

%%

/* ==================================================== EXPRESSIONS ==================================================== */

primary_expression
	: IDENTIFIER {
		auto ptr = symbolTable.retrieve_ordinary($1);
		if (!ptr)
		{
			std::cerr << "Undefined symbol " << $1 << std::endl;/*todo: error*/
			YYERROR;
		}
		if (ptr->storage == SymbolTable::Ordinary::EXTERN)
			$$ = {Expression::IDENTIFIER, {}, std::nullopt, std::nullopt, $1, ptr->type, ptr}; // todo: understand
		else if (ptr->storage == SymbolTable::Ordinary::STATIC)
			$$ = {Expression::IDENTIFIER, {}, std::nullopt, std::nullopt, $1, ptr->type, ptr};
		else
			$$ = {Expression::IDENTIFIER, {}, std::nullopt, std::nullopt, $1, ptr->type, ptr};
		$$.type->is_lvalue = true;
	} /*todo*/
	| CONSTANT {$$ = {Expression::CONSTANT, {}, std::nullopt, $1, {}, CTYPE_LONG_INT, $1};}
	| FLOAT_CONSTANT {$$ = {Expression::CONSTANT, {}, std::nullopt, $1, {}, CTYPE_LONG_DOUBLE, $1};}
	| STRING_LITERAL {$$ = {Expression::CONSTANT, {}, std::nullopt, $1, {}, CTYPE_CHAR_PTR, $1};}
	| '(' expression ')' {$$ = $2;}
	;

postfix_expression
	: primary_expression {$$ = $1;}
	| postfix_expression '[' expression ']' {$$ = {Expression::DEREFERENCE, {{Expression::ADD, {$1, $3}}}};}
	| postfix_expression '(' ')' {$$ = {Expression::CALL, {$1}};}
	| postfix_expression '(' argument_expression_list ')' {$$ = {Expression::CALL, {$1}}; $$.operands.insert($$.operands.end(), $3.begin(), $3.end());}
	| postfix_expression '.' IDENTIFIER {$$ = {Expression::MEMBER_ACCESS, {$1}, std::nullopt, {}, $3};}
	| postfix_expression PTR_OP IDENTIFIER {$$ = {Expression::PTR_MEMBER_ACCESS, {$1}, std::nullopt, {}, $3};}
	| postfix_expression INC_OP {$$ = {Expression::POST_INC, {$1}};}
	| postfix_expression DEC_OP {$$ = {Expression::POST_DEC, {$1}};}
	;

argument_expression_list
	: assignment_expression {$$ = {$1};}
	| argument_expression_list ',' assignment_expression {$1.emplace_back($3);$$ = $1; }
	;

unary_expression
	: postfix_expression {$$ = $1;}
	| INC_OP unary_expression {$$ = {Expression::PRE_INC, {$2}};}
	| DEC_OP unary_expression {$$ = {Expression::PRE_DEC, {$2}};}
	| unary_operator cast_expression {$$ = {$1, {$2}};}
	| SIZEOF unary_expression {
		Expression::DeduceType($2);
		const SymbolTable::Constant* constant = symbolTable.new_constant(symbolTable.size_of($2.type.value()));
		$$ = {
			Expression::CONSTANT,
			{},
			std::nullopt,
			constant,
			{},
			CTYPE_UNSIGNED_LONG_INT,
			constant
		};
	}
	| SIZEOF '(' type_name ')' {
		const SymbolTable::Constant* constant = symbolTable.new_constant(symbolTable.size_of(Ast::parse_type_name($3)));
		$$ = {
			Expression::CONSTANT,
			{},
			std::nullopt,
			constant,
			{},
			CTYPE_UNSIGNED_LONG_INT,
			constant
		};
	}
	;

unary_operator
	: '&' {$$ = Expression::ADDRESS;}
	| '*' {$$ = Expression::DEREFERENCE;}
	| '+' {$$ = Expression::UPLUS;}
	| '-' {$$ = Expression::UMINUS;}
	| '~' {$$ = Expression::BITWISE_NOT;}
	| '!' {$$ = Expression::LOGICAL_NOT;}
	;

cast_expression
	: unary_expression {$$ = $1;}
	| '(' type_name ')' cast_expression {$$ = {Expression::CAST, {$4}, Ast::parse_type_name($2)};}
	;

multiplicative_expression
	: cast_expression {$$ = $1;}
	| multiplicative_expression '*' cast_expression {$$ = {Expression::MUL, {$1, $3}};}
	| multiplicative_expression '/' cast_expression {$$ = {Expression::DIV, {$1, $3}};}
	| multiplicative_expression '%' cast_expression {$$ = {Expression::MOD, {$1, $3}};}
	;

additive_expression
	: multiplicative_expression {$$ = $1;}
	| additive_expression '+' multiplicative_expression {$$ = {Expression::ADD, {$1, $3}};}
	| additive_expression '-' multiplicative_expression {$$ = {Expression::SUB, {$1, $3}};}
	;

shift_expression
	: additive_expression {$$ = $1;}
	| shift_expression LEFT_OP additive_expression {$$ = {Expression::BITSHIFT_LEFT, {$1, $3}};}
	| shift_expression RIGHT_OP additive_expression {$$ = {Expression::BITSHIFT_RIGHT, {$1, $3}};}
	;

relational_expression
	: shift_expression {$$ = $1;}
	| relational_expression '<' shift_expression {$$ = {Expression::LESSER, {$1, $3}};}
	| relational_expression '>' shift_expression {$$ = {Expression::GREATER, {$1, $3}};}
	| relational_expression LE_OP shift_expression {$$ = {Expression::LESSER_EQUAL, {$1, $3}};}
	| relational_expression GE_OP shift_expression {$$ = {Expression::GREATER_EQUAL, {$1, $3}};}
	;

equality_expression
	: relational_expression {$$ = $1;}
	| equality_expression EQ_OP relational_expression {$$ = {Expression::EQUAL, {$1, $3}};}
	| equality_expression NE_OP relational_expression {$$ = {Expression::NOT_EQUAL, {$1, $3}};}
	;

and_expression
	: equality_expression {$$ = $1;}
	| and_expression '&' equality_expression {$$ = {Expression::BITWISE_AND, {$1, $3}};}
	;

exclusive_or_expression
	: and_expression {$$ = $1;}
	| exclusive_or_expression '^' and_expression {$$ = {Expression::BITWISE_XOR, {$1, $3}};}
	;

inclusive_or_expression
	: exclusive_or_expression {$$ = $1;}
	| inclusive_or_expression '|' exclusive_or_expression {$$ = {Expression::BITWISE_OR, {$1, $3}};}
	;

logical_and_expression
	: inclusive_or_expression {$$ = $1;}
	| logical_and_expression AND_OP inclusive_or_expression {$$ = {Expression::LOGICAL_AND, {$1, $3}};}
	;

logical_or_expression
	: logical_and_expression {$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression {$$ = {Expression::LOGICAL_OR, {$1, $3}};}
	;

conditional_expression
	: logical_or_expression {$$ = $1;}
	| logical_or_expression '?' expression ':' conditional_expression {$$ = {Expression::TERNARY, {$1, $3, $5}};}
	;

assignment_expression
	: conditional_expression {$$ = $1;}
	| unary_expression assignment_operator assignment_expression {$$ = {$2, {$1, $3}};}
	;

assignment_operator
	: '=' {$$ = Expression::ASSIGNMENT;}
	| MUL_ASSIGN {$$ = Expression::MUL_ASSIGNMENT;}
	| DIV_ASSIGN {$$ = Expression::DIV_ASSIGNMENT;}
	| MOD_ASSIGN {$$ = Expression::MOD_ASSIGNMENT;}
	| ADD_ASSIGN {$$ = Expression::ADD_ASSIGNMENT;}
	| SUB_ASSIGN {$$ = Expression::SUB_ASSIGNMENT;}
	| LEFT_ASSIGN {$$ = Expression::BITSHIFT_LEFT_ASSIGNMENT;}
	| RIGHT_ASSIGN {$$ = Expression::BITSHIFT_RIGHT_ASSIGNMENT;}
	| AND_ASSIGN {$$ = Expression::BITWISE_AND_ASSIGNMENT;}
	| XOR_ASSIGN {$$ = Expression::BITWISE_XOR_ASSIGNMENT;}
	| OR_ASSIGN {$$ = Expression::BITWISE_OR_ASSIGNMENT;}
	;

comma_expression
	: assignment_expression {$$ = $1;}
	| comma_expression ',' assignment_expression {$$ = {Expression::COMMA, {$1, $3}};}
	;
	
expression
	: comma_expression {Expression::DeduceType($1);$$ = $1;}
	;

constant_expression
	: expression {
		if (!Ast::is_const_expression($1))
		{
			yyerror("expression is not constant");
			YYERROR;
		}
		$$ = $1;
	}
	;

declaration
	: declaration_specifier_list ';' {$$ = {$1, std::nullopt}; }
	| declaration_specifier_list init_declarator_list ';' {$$ = {$1, $2}; }
	;
	
declaration_specifier
	: type_specifier {$$ = $1;}
	| storage_class_specifier {$$ = $1;}
	| type_qualifier {$$ = $1;}
	;


declaration_specifier_list
	: declaration_specifier {$$ = Ast::DeclarationSpecifierList(1, $1); current_declaration_specifier_list = $$;}
	| declaration_specifier_list declaration_specifier  {$$ = $1; $$.emplace_back($2); current_declaration_specifier_list = $$;}
	;

init_declarator_list
	: init_declarator {$$ = Ast::InitDeclaratorList(1, $1);Ast::Declare(Ast::Declaration{$<Ast::DeclarationSpecifierList>0, {{$1}}});}
	| init_declarator_list ',' init_declarator {$$ = $1; $$.emplace_back($3);Ast::Declare(Ast::Declaration{$<Ast::DeclarationSpecifierList>0, {{$3}}});}
	;

init_declarator
	: declarator {$$ = {$1, std::nullopt};}
	| declarator '=' initializer {$$ = {$1, $3};}
	;

storage_class_specifier
	: TYPEDEF {$$ = Ast::StorageSpecifier::TYPEDEF;}
	| EXTERN {$$ = Ast::StorageSpecifier::EXTERN;}
	| STATIC {$$ = Ast::StorageSpecifier::STATIC;}
	| AUTO {$$ = Ast::StorageSpecifier::AUTO;}
	| REGISTER {$$ = Ast::StorageSpecifier::REGISTER;}
	;

type_specifier
	: VOID {$$ =  {Ast::TypeSpecifier::VOID};}
	| CHAR {$$ =  {Ast::TypeSpecifier::CHAR};}
	| SHORT {$$ =  {Ast::TypeSpecifier::SHORT};}
	| INT {$$ =  {Ast::TypeSpecifier::INT};}
	| LONG {$$ =  {Ast::TypeSpecifier::LONG};}
	| FLOAT {$$ =  {Ast::TypeSpecifier::FLOAT};}
	| DOUBLE {$$ =  {Ast::TypeSpecifier::DOUBLE};}
	| SIGNED {$$ =  {Ast::TypeSpecifier::SIGNED};}
	| UNSIGNED {$$ =  {Ast::TypeSpecifier::UNSIGNED};}
	| struct_or_union_specifier {$$ =  {Ast::TypeSpecifier::STRUCT_OR_UNION, $1};}
	| enum_specifier {$$ = {Ast::TypeSpecifier::ENUM, $1};}
	| TYPE_NAME {$$ =  {Ast::TypeSpecifier::TYPE_NAME, $1};}
	;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}' {$$ = {$1, $2, $4};}
	| struct_or_union '{' struct_declaration_list '}' {$$ = {$1, std::nullopt, $3};}
	| struct_or_union IDENTIFIER {$$ = {$1, $2, std::nullopt};}
	;

struct_or_union
	: STRUCT {$$ = Ast::StructOrUnion::STRUCT;}
	| UNION {$$ = Ast::StructOrUnion::UNION;}
	;

struct_declaration_list
	: struct_declaration {$$ = {std::shared_ptr<Ast::StructDeclaration>(new Ast::StructDeclaration($1))};}
	| struct_declaration_list struct_declaration {$$ = $1; $$.emplace_back(new Ast::StructDeclaration($2));}
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';' {$$ = {$1, $2};}
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list {$$ = $2; $$.emplace_front($1);}
	| type_specifier {$$ = {$1};}
	| type_qualifier specifier_qualifier_list {$$ = $2; $$.emplace_front($1);}
	| type_qualifier {$$ = {$1};}
	;

struct_declarator_list
	: struct_declarator {$$ = {$1};}
	| struct_declarator_list ',' struct_declarator {$$ = $1; $$.emplace_back($3);}
	;

struct_declarator
	: declarator {$$ = {$1, std::nullopt};}
	| ':' constant_expression {$$ = {std::nullopt, $2};}
	| declarator ':' constant_expression {$$ = {$1, $3};}
	;

enum_specifier
	: ENUM IDENTIFIER '{' enumerator_list '}' {$$ = {$2, $4};}
	| ENUM '{' enumerator_list '}' {$$ = {std::nullopt, $3};}
	| ENUM IDENTIFIER {$$ = {$2, std::nullopt};}
	;

enumerator_list
	: enumerator {$$ = Ast::EnumeratorList(1, $1);}
	| enumerator_list ',' enumerator {$$ = $1; $$.emplace_back($3);}
	;

enumerator
	: IDENTIFIER {$$ = {$1, std::nullopt};}
	| IDENTIFIER '=' constant_expression {$$ = {$1, 0/*Expression::evaluate_constant_expression($3) todo enum*/};}
	;

type_qualifier
	: CONST {$$ = Ast::TypeQualifier::CONST;}
	| VOLATILE {$$ = Ast::TypeQualifier::VOLATILE;}
	;

declarator
	: pointer direct_declarator {$$ = {$1, $2};}
	| direct_declarator {$$ = {{}, $1};}
	;

array_declarator
	: direct_declarator_pointer '[' constant_expression ']' {$$ = {$1, $3};}
	| direct_declarator_pointer '[' ']' {$$ = {$1, std::nullopt};}
	;

function_declarator
	: direct_declarator_pointer '(' parameter_type_list ')' {$$ = {$1, $3};}
	| direct_declarator_pointer '(' identifier_list ')' {$$ = {$1, $3};}
	| direct_declarator_pointer '(' ')' {$$ = {$1};}
	;

nested_declarator
	: '(' declarator ')' {$$ = Ast::NestedDeclarator(new Ast::Declarator($2));}
	;

direct_declarator
	: IDENTIFIER {$$ = $1;}
	| TYPE_NAME {$$ = $1;}
	| nested_declarator {$$ = $1;}
	| array_declarator {$$ = $1;}
	| function_declarator {$$ = $1;}
	;

direct_declarator_pointer
	: direct_declarator {$$ = Ast::DirectDeclaratorPointer(new Ast::DirectDeclarator($1));}
	;

pointer
	: '*' {$$ = {std::nullopt};}
	| '*' type_qualifier_list {$$ = {$2};}
	| '*' pointer {$$ = $2; $$.emplace_front(std::nullopt);}
	| '*' type_qualifier_list pointer {$$ = $3; $$.emplace_front($2);}
	;

type_qualifier_list
	: type_qualifier {$$ = {$1};}
	| type_qualifier_list type_qualifier {$$ = $1 | $2;}
	;


parameter_type_list
	: parameter_list {$$ = {$1, false};}
	| parameter_list ',' ELLIPSIS {$$ = {$1, true};}
	;

parameter_list
	: parameter_declaration {$$ = {$1};}
	| parameter_list ',' parameter_declaration {$$ = $1; $$.emplace_back($3);}
	;

parameter_declaration
	: declaration_specifier_list declarator {$$ = {$1, Ast::NestedDeclarator(new Ast::Declarator($2))};}
	| declaration_specifier_list abstract_declarator {$$ = {$1, Ast::NestedDeclarator(new Ast::Declarator($2))};}
	| declaration_specifier_list {$$ = {$1, {}};}
	;

identifier_list
	: IDENTIFIER {$$ = {$1};}
	| identifier_list ',' IDENTIFIER {$$ = $1; $$.emplace_back($3);}
	;

type_name
	: specifier_qualifier_list {$$ = {$1, std::nullopt};}
	| specifier_qualifier_list abstract_declarator {$$ = {$1, $2};}
	;

abstract_declarator
	: pointer {$$ = {$1};}
	| direct_abstract_declarator {$$ = {{}, $1};}
	| pointer direct_abstract_declarator {$$ = {$1, $2};}
	;

array_abstract_declarator
	: '[' ']' {$$ = {nullptr, std::nullopt};}
	| '[' constant_expression ']' {$$ = {nullptr, $2};}
	| direct_abstract_declarator_pointer '[' ']' {$$ = {$1, std::nullopt};}
	| direct_abstract_declarator_pointer '[' constant_expression ']' {$$ = {$1, $3};}
	;

function_abstract_declarator
	: '(' ')' {$$ = {nullptr};}
	| '(' parameter_type_list ')' {$$ = {nullptr, $2};}
	| direct_abstract_declarator_pointer '(' ')' {$$ = {$1, {}};}
	| direct_abstract_declarator_pointer '(' parameter_type_list ')' {$$ = {$1, $3};}
	;

nested_abstract_declarator
	: '(' abstract_declarator ')' {$$ = Ast::NestedDeclarator(new Ast::Declarator($2));}
	;

direct_abstract_declarator
	: function_abstract_declarator {$$ = $1;}
	| array_abstract_declarator {$$ = $1;}
	| nested_abstract_declarator {$$ = $1;}
	;

direct_abstract_declarator_pointer
	: direct_abstract_declarator {$$ = Ast::DirectDeclaratorPointer(new Ast::DirectDeclarator($1));}
	;

initializer
	: assignment_expression {$$ = {$1};}
	| '{' initializer_list '}' {$$ = {$2};}
	| '{' initializer_list ',' '}' {$$ = {$2};}
	;

initializer_list
	: initializer {$$ = {$1};}
	| initializer_list ',' initializer {$$ = $1; $$.emplace_back($3);}
	;

statement
	: labeled_statement {$$ = {$1};}
	| enter_block compound_statement exit_block {$2.scope = $1;$$ = {$2};}
	| expression_statement {$$ = {$1};}
	| selection_statement {$$ = {$1};}
	| iteration_statement {$$ = {$1};}
	| jump_statement {$$ = {$1};}
	;

statement_ptr
	: statement {$$ = Ast::StatementPtr(new Ast::Statement($1));}
	;

labeled_statement
	: IDENTIFIER ':' statement_ptr {$$ = {$1, $3};}
	| CASE constant_expression ':' statement_ptr {$$ = {$2, $4};}
	| DEFAULT ':' statement_ptr {$$ = {{}, $3};}
	;

compound_statement
	: '{'  '}' {$$ = {-1, std::nullopt, std::nullopt};}
	| '{'  statement_list '}' {$$ = {-1, std::nullopt, $2};}
	| '{'  declaration_list_declared '}' {$$ = {-1, $2, std::nullopt};}
	| '{'  declaration_list_declared statement_list '}' {$$ = {-1, $2, $3};}
	;

enter_block: {$$ = symbolTable.enter_block();} ;
exit_block: {symbolTable.exit_block();} ;

declaration_list
	: declaration {$$ = {$1};}
	| declaration_list declaration {($$ = $1).emplace_back($2);}
	;

declaration_list_declared
	: declaration {$$ = {$1};/*Ast::Declare($$.back());*/}
	| declaration_list_declared declaration {($$ = $1).emplace_back($2);/*Ast::Declare($$.back());*/}
	;

statement_list
	: statement {$$ = {$1};}
	| statement_list statement {($$ = $1).emplace_back($2);}
	;

expression_statement
	: ';'  {$$ = {std::nullopt};}
	| expression ';' {$$ = {$1};Expression::EmitTac($1);}
	;

else
	: ELSE
	{
		$<TAC::Label>$ = TAC::currentFunction->new_label();
		TAC::currentFunction->add_instruction({$<TAC::Label>$, TAC::JUMP});
		TAC::currentFunction->set_label($<TAC::Label>-1);
	}
	statement_ptr
	{
		TAC::currentFunction->set_label($<TAC::Label>2);
		$$ = $3;
	}
	|
	{
		TAC::currentFunction->set_label($<TAC::Label>-1);
		$$ = {};
	}
	;

selection_statement
	: IF '(' expression ')'
	{
		Expression::EmitTac($3);
		$<TAC::Label>$ = TAC::currentFunction->new_label();
		TAC::currentFunction->add_instruction({$<TAC::Label>$, TAC::JUMP_EQUAL, $3.ret_address, symbolTable.new_constant(0), symbolTable.size_of($3.type.value()), Types::is_signed($3.type.value())});
	}
	statement_ptr
	else
	{$$ = {Ast::SelectionStatement::IF, $3, $6, $7};}
	| SWITCH '(' expression ')' statement_ptr {$$ = {Ast::SelectionStatement::SWITCH, $3, $5};}
	;

iteration_statement
	: WHILE '('
	{
		$<TAC::Label>$ = TAC::currentFunction->new_label(true);
	}
	expression ')'
	{
		Expression::EmitTac($4);
		$<TAC::Label>$ = TAC::currentFunction->new_label();
		TAC::currentFunction->add_instruction({$<TAC::Label>$, TAC::JUMP_EQUAL, $4.ret_address, symbolTable.new_constant(0), symbolTable.size_of($4.type.value()), Types::is_signed($4.type.value())});
	}
	statement_ptr
	{
		TAC::currentFunction->add_instruction({$<TAC::Label>3, TAC::JUMP});
		//std::cout << int($<TAC::Label>6) << std::endl;
		TAC::currentFunction->set_label($<TAC::Label>6);
	}
	{$$ = {Ast::IterationStatement::WHILE, $4, $7};}

	| DO {$<TAC::Label>$ = TAC::currentFunction->new_label(true);} statement_ptr WHILE '(' expression ')' ';'
	{
		Expression::EmitTac($6);
	//std::cout << "[label " << int($<TAC::Label>2) << "]" << std::endl;
		$$ = {Ast::IterationStatement::DO_WHILE, $6, $3};
		TAC::currentFunction->add_instruction({$<TAC::Label>2, TAC::JUMP_NOT_EQUAL, $6.ret_address, symbolTable.new_constant(0)});
	}
	| FOR '(' expression_statement expression_statement ')' statement_ptr {$$ = {Ast::IterationStatement::FOR, $4.expression, $6, $3.expression};}
	| FOR '(' expression_statement expression_statement expression ')' statement_ptr {$$ = {Ast::IterationStatement::FOR, $4.expression, $7, $3.expression, $5};}
	;

jump_statement
	: GOTO IDENTIFIER ';' {$$ = {Ast::JumpStatement::GOTO, $2};}
	| CONTINUE ';' {$$ = {Ast::JumpStatement::CONTINUE};}
	| BREAK ';' {$$ = {Ast::JumpStatement::BREAK};}
	| RETURN ';' {$$ = {Ast::JumpStatement::RETURN};TAC::currentFunction->add_instruction({{}, TAC::RETURN});}
	| RETURN expression ';' {
		Expression::EmitTac($2);
		$$ = {Ast::JumpStatement::RETURN, $2};TAC::currentFunction->add_instruction({{}, TAC::RETURN, $2.ret_address, {}, symbolTable.size_of($2.type.value()), Types::is_signed($2.type.value())});
	}
	;

translation_unit
	: external_declaration {tree = {$1};}
	| translation_unit external_declaration {tree.emplace_back($2);}
	;

external_declaration
	: function_definition {$$ = $1;}
	| declaration {$$ = $1;/*Ast::Declare($1);*/}
	;

function_declaration_specifier_list
	: declaration_specifier_list {symbolTable.enter_block();symbolTable.enter_prototype(); $$ = $1;}
	|  {symbolTable.enter_block();symbolTable.enter_prototype(); $$ = std::nullopt;}
	;

function_definition_declaration
	: declarator
	{
		$$ = {std::nullopt, $1};
		symbolTable.enter_block();
		symbolTable.enter_prototype();
		Ast::DeclareFunction($$);
		TAC::functions.emplace_back(symbolTable.get_current_function());
		TAC::currentFunction = &TAC::functions.back();
		symbolTable.get_current_function().tac = TAC::currentFunction;
		symbolTable.exit_prototype();
	}
	| declarator declaration_list
	{
		$$ = {std::nullopt, $1};
		symbolTable.enter_block();
		symbolTable.enter_prototype();
		Ast::Declare($2);
		Ast::DeclareFunction($$);
		TAC::functions.emplace_back(symbolTable.get_current_function());
		TAC::currentFunction = &TAC::functions.back();
		symbolTable.get_current_function().tac = TAC::currentFunction;
		symbolTable.exit_prototype();
	}
	| declaration_specifier_list declarator
	{
		$$ = {$1, $2};
		symbolTable.enter_block();
		symbolTable.enter_prototype();
		Ast::DeclareFunction($$);
		TAC::functions.emplace_back(symbolTable.get_current_function());
		TAC::currentFunction = &TAC::functions.back();
		symbolTable.get_current_function().tac = TAC::currentFunction;
		symbolTable.exit_prototype();
	}
	| declaration_specifier_list declarator declaration_list
	{
		$$ = {$1, $2};
		symbolTable.enter_block();
		symbolTable.enter_prototype();
		Ast::Declare($3);
		Ast::DeclareFunction($$);
		TAC::functions.emplace_back(symbolTable.get_current_function());
		TAC::currentFunction = &TAC::functions.back();
		symbolTable.get_current_function().tac = TAC::currentFunction;
		symbolTable.exit_prototype();
	}
	;

function_definition
	: function_definition_declaration compound_statement {$$ = {$1, std::nullopt, $2};symbolTable.exit_function();symbolTable.exit_block();}
	;

enter_function: {$$ = symbolTable.enter_block();symbolTable.enter_prototype();};

%%

#include <stdio.h>
#include <regex>
#include "Diagnostic.hpp"

extern char yytext[];
Ast::DeclarationSpecifierList current_declaration_specifier_list;

int yyerror(char *s)
{
	return Diagnostic::Error(s, Diagnostic::Error::ERROR).print();
}

int yyerror(const std::string &s)
{
	return yyerror(s.c_str());
}