#ifndef CC1_POC_DIAGNOSTIC_HPP
#define CC1_POC_DIAGNOSTIC_HPP
#include <string>

class Diagnostic {
public:
	struct Error {
		enum Severity {
			NOTICE,
			WARNING,
			ERROR
		};
		std::string	file;
		std::string	full_line;
		size_t		lineno;
		size_t		column;
		std::string	description;
		Severity	severity;

		Error(std::string f, std::string fl, size_t l, size_t c, std::string d, Severity s);
		Error(std::string description, Error::Severity severity);
		int print() const;
	};

	void	emit_error(std::string description, Error::Severity severity);

};


#endif
