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

class SpinIntegrator {
public:
	SpinIntegrator() = default;

	const std::vector< Terms::IndexSubstitution > &spinIntegrate(const Terms::Term &term);

protected:
	std::vector< Terms::IndexSubstitution > m_substitutions;

	void process(const Terms::Tensor &tensor);
	void process(const IndexGroup &group, int targetDoubleMs);
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_SPININTEGRATOR_HPP_
