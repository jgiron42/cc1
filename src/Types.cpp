#include "Types.hpp"

namespace Types {
	bool is_signed(const CType &t)
	{
		if (!holds_alternative<PlainType>(t.type))
			return false;
		return get<PlainType>(t.type).is_signed;
	}
}