#ifndef CC1_POC_TYPES_HPP
#define CC1_POC_TYPES_HPP
#include <variant>
#include <string>
#include <memory>
#include <optional>
#include <map>
#include <queue>
#include <list>

namespace Types
{
	struct PlainType {
		friend bool operator==(const PlainType &, const PlainType &) = default;
		enum BaseType{
			VOID,
			CHAR,
			SHORT_INT,
			INT,
			LONG_INT,
			FLOAT,
			DOUBLE,
			LONG_DOUBLE
		};
		BaseType	base;
		bool 		is_signed;
	};
	struct CType;
	struct StructOrUnion {
		friend bool operator==(const StructOrUnion &, const StructOrUnion &) = default;
		struct Entry {
			std::string					name;
			std::shared_ptr<CType>		type;
			int							bitfield;
		};
		bool							is_union;
		std::optional<std::string>		tag;
		std::deque<Entry>				members;
		std::map<std::string, Entry*>	member_map;
	};
	struct Enum {

	};

	struct Tag {
		friend bool operator==(const Tag &, const Tag &) = default;
		enum Type {
			ENUM,
			STRUCT,
			UNION
		} type;
		std::variant<
		std::monostate,
		std::string,
		StructOrUnion,
		Enum
		> declaration;
	};

	struct TagName {
		friend bool operator==(const TagName &, const TagName &) = default;
		typedef Tag::Type Type;
		Type type;
		std::string name;
	};

	struct Pointer {
		friend bool operator==(const Pointer &, const Pointer &) = default;
		std::shared_ptr<CType> pointed_type;
	};
	struct FunctionType {
		friend bool operator==(const FunctionType &, const FunctionType &) = default;
		std::shared_ptr<CType>	return_type;
		std::list<CType>		parameters;
		bool					variadic;
	};
	struct Array {
		friend bool operator==(const Array &, const Array &) = default;
		std::shared_ptr<CType>	value_type;
		std::optional<int>		size;
	};
	struct Typename {
		friend bool operator==(const Typename &, const Typename &) = default;
		std::string name;
	};
	struct CType {
		enum TypeQualifier {
			NONE = 0,
			VOLATILE = 1,
			CONST = 2
		};
		friend inline TypeQualifier operator|(const TypeQualifier &a, const TypeQualifier &b)
		{ return static_cast<TypeQualifier>(static_cast<int>(a) | static_cast<int>(b)); }
		friend inline TypeQualifier operator|=(TypeQualifier &a, const TypeQualifier &b) { return (a = (a | b)); }
		friend bool operator==(const CType &, const CType &) = default;
		std::variant<
		PlainType,
		TagName,
		Pointer,
		FunctionType,
		Array,
		Typename
		> type;
		TypeQualifier	qualifier;
		bool			is_lvalue;
	};

	bool is_signed(const CType &t);
}
#endif //CC1_POC_TYPES_HPP
