#ifndef CONTRACTOR_PROCESSOR_PRINTERWRAPPER_HPP_
#define CONTRACTOR_PROCESSOR_PRINTERWRAPPER_HPP_

#include "formatting/PrettyPrinter.hpp"

namespace Contractor::Processor {
class PrinterWrapper {
public:
	PrinterWrapper(Formatting::PrettyPrinter *printer = nullptr) : m_printer(printer) {}
	PrinterWrapper(Formatting::PrettyPrinter &printer) : m_printer(&printer) {}

	template< typename T > PrinterWrapper &operator<<(const T &msg) {
		if (m_printer) {
			*m_printer << msg;
		}

		return *this;
	}

protected:
	Formatting::PrettyPrinter *m_printer;
};
}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_PRINTERWRAPPER_HPP_
