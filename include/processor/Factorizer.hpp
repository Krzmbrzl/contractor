#ifndef CONTRACTOR_PROCESSOR_FACTORIZER_HPP_
#define CONTRACTOR_PROCESSOR_FACTORIZER_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <unordered_map>
#include <vector>

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Processor {

class Factorizer {
public:
	Factorizer(const Utils::IndexSpaceResolver &resolver);

	const std::vector< Terms::BinaryTerm > &factorize(const Terms::GeneralTerm &term);

	Terms::ContractionResult::cost_t getLastFactorizationCost() const;

	Terms::ContractionResult::cost_t getLastBiggestIntermediateSize() const;

	/**
	 * @returns The formal scaling of the resulting factorization. The formal scaling contains the highest
	 * exponent of each index space's size that will appear in any of the binary terms in the produced factorization.
	 */
	const std::unordered_map< Terms::IndexSpace, unsigned int > getLastFormalScaling() const;

protected:
	const Utils::IndexSpaceResolver &m_resolver;
	Terms::ContractionResult::cost_t m_bestCost                = 0;
	Terms::ContractionResult::cost_t m_biggestIntermediateSize = 0;
	std::vector< Terms::BinaryTerm > m_bestFactorization;
	std::unordered_map< Terms::IndexSpace, unsigned int > m_bestScaling;

	bool doFactorize(const Terms::ContractionResult::cost_t &costSoFar,
					 const std::unordered_map< Terms::IndexSpace, unsigned int > &formalScaling,
					 const Terms::ContractionResult::cost_t &biggestIntermediate, std::vector< Terms::Tensor > &tensors,
					 std::vector< Terms::BinaryTerm > &factorizedTerms, const Terms::GeneralTerm &term);
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_FACTORIZER_HPP_
