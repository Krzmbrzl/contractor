#ifndef CONTRACTOR_PROCESSOR_SPININTEGRATOR_HPP_
#define CONTRACTOR_PROCESSOR_SPININTEGRATOR_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSubstitution.hpp"

#include <vector>

namespace Contractor::Terms {
class Term;
class Tensor;
}; // namespace Contractor::Terms

namespace Contractor::Processor {

struct IndexGroup {
	std::vector< Terms::Index > creator;
	std::vector< Terms::Index > annihilator;
};

/**
 * A class taking care of performing spin-integration
 */
class SpinIntegrator {
public:
	SpinIntegrator() = default;

	/**
	 * Carries out spin-integration on the given Term
	 *
	 * @param term The Term to intgrate
	 * @returns A list of substitutions that represent the non-zero spin-cases for the
	 * given Term (applying the substitution to the term results in the respective spin case)
	 */
	const std::vector< Terms::IndexSubstitution > &spinIntegrate(const Terms::Term &term);

protected:
	std::vector< Terms::IndexSubstitution > m_substitutions;

	void process(const Terms::Tensor &tensor);
	void process(const IndexGroup &group);
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_SPININTEGRATOR_HPP_
