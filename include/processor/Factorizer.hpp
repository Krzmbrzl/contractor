#ifndef CONTRACTOR_PROCESSOR_FACTORIZER_HPP_
#define CONTRACTOR_PROCESSOR_FACTORIZER_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp" // ContractionResult

#include <vector>

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Processor {

class Factorizer {
public:
	Factorizer(const Utils::IndexSpaceResolver &resolver);

	std::vector< Terms::BinaryTerm > factorize(const Terms::GeneralTerm &term);

	Terms::ContractionResult::cost_t getLastFactorizationCost() const;

protected:
	const Utils::IndexSpaceResolver &m_resolver;
	Terms::ContractionResult::cost_t m_bestCost = 0;
	std::vector< Terms::BinaryTerm > m_bestFactorization;

	bool doFactorize(const Terms::ContractionResult::cost_t &costSoFar, std::vector< Terms::Tensor > &tensors,
					 std::vector< Terms::BinaryTerm > &factorizedTerms, const Terms::GeneralTerm &term);
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_FACTORIZER_HPP_
