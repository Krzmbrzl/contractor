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
	 * @param calculatesEndResult Whether this term calculates an end-result Tensor. For these potentially only specific
	 * spin cases are relevant and thus all other spin cases will be stripped out.
	 * @returns A list of substitutions that represent the non-zero spin-cases for the
	 * given Term (applying the substitution to the term results in the respective spin case)
	 */
	const std::vector< Terms::IndexSubstitution > &spinIntegrate(const Terms::Term &term, bool calculatesEndResult);

protected:
	std::vector< Terms::IndexSubstitution > m_substitutions;

	void process(const Terms::Tensor &tensor);
	void process(const IndexGroup &group);

	/**
	 * Adds the hard-coded spin-substitutions for the given result Tensor to the list of
	 * current substitutions (if any). These hard-coded substitutions are used because we know in advance
	 * that these are the only relevant spin cases for this Tensor.
	 *
	 * @param resultTensor The Tensor which to spin-integrate in a hard-coded way
	 * @returns Whether hard-coded substitutions for this Tensor are available (and thus have been added)
	 */
	bool useHardcodedResultSpinCases(const Terms::Tensor &resultTensor);
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_SPININTEGRATOR_HPP_
