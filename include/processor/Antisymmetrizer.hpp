#ifndef CONTRACTOR_PROCESSOR_ANTISYMMETRIZER_HPP_
#define CONTRACTOR_PROCESSOR_ANTISYMMETRIZER_HPP_

#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "terms/Term.hpp"
#include "utils/HeapsAlgorithm.hpp"

#include <type_traits>
#include <unordered_set>
#include <vector>

#include <boost/range/join.hpp>

namespace Contractor::Processor {

template< typename term_t > class Antisymmetrizer {
public:
	Antisymmetrizer()  = default;
	~Antisymmetrizer() = default;

	const std::vector< term_t > &antisymmetrize(const term_t &term) {
		static_assert(std::is_base_of_v< Terms::Term, term_t >);

		m_resultingTerms.clear();

		std::vector< Terms::Index > creators;
		std::vector< Terms::Index > annihilators;

		for (const Terms::Index &current : term.getResult().getIndices()) {
			switch (current.getType()) {
				case Terms::Index::Type::Creator:
					creators.push_back(current);
					break;
				case Terms::Index::Type::Annihilator:
					annihilators.push_back(current);
					break;
				case Terms::Index::Type::None:
					// Do nothing
					break;
			}
		}

		std::vector< Terms::IndexSubstitution > creatorSubs     = antisymmetrize(creators, term.getResult());
		std::vector< Terms::IndexSubstitution > annihilatorSubs = antisymmetrize(annihilators, term.getResult());

		Terms::PermutationGroup symmetry = term.getResult().getSymmetry();
		for (const Terms::IndexSubstitution &current : boost::join(creatorSubs, annihilatorSubs)) {
			if (!current.isIdentity()) {
				symmetry.addGenerator(current, false);
			}
		}

		// Create all possible combinations of the both substitution categories
		for (const Terms::IndexSubstitution &currentCreatorSub : creatorSubs) {
			for (const Terms::IndexSubstitution &currentAnnihilatorSub : annihilatorSubs) {
				term_t termCopy = term;

				Terms::IndexSubstitution::factor_t factor = 1;

				// Now also apply it to the rest of the Tensors in the Term
				for (Terms::Tensor &currentTensor : termCopy.accessTensors()) {
					factor = 1;
					factor *= currentCreatorSub.apply(currentTensor);
					factor *= currentAnnihilatorSub.apply(currentTensor);
				}

				termCopy.setPrefactor(factor * term.getPrefactor());

				symmetry.setRootSequence(termCopy.getResult().getIndices());
				termCopy.accessResult().setSymmetry(symmetry);

				m_resultingTerms.push_back(std::move(termCopy));
			}
		}

		assert(!m_resultingTerms.empty());
		return m_resultingTerms;
	}

	double getPrefactor() const { return 1.0 / m_resultingTerms.size(); }

	std::vector< Terms::IndexSubstitution > antisymmetrize(const std::vector< Terms::Index > &indices,
														   const Terms::Tensor &tensor) {
		std::vector< Terms::IndexSubstitution > substitutions;

		// The identity substitution should always be present
		substitutions.push_back(Terms::IndexSubstitution::identity());

		std::vector< Terms::Index > indicesCopy = indices;
		Utils::HeapsAlgorithm heapAlg(indicesCopy);

		do {
			Terms::IndexSubstitution::substitution_list currentSubs;
			std::unordered_set< Terms::Index > processedIndices;

			// Create index mapping from the original indices to the current permutation
			for (std::size_t i = 0; i < indices.size(); ++i) {
				if (processedIndices.find(indices[i]) == processedIndices.end()
					&& processedIndices.find(indicesCopy[i]) == processedIndices.end()) {
					currentSubs.push_back(Terms::IndexSubstitution::index_pair_t(indices[i], indicesCopy[i]));

					processedIndices.insert(indices[i]);
					processedIndices.insert(indicesCopy[i]);
				} else {
#ifndef NDEBUG
					// Verify that the previous mapping is the reverse of the current one. Everything else is unexpected
					bool duplicateSubstitutionIsReverseOfExisting = false;
					for (const Terms::IndexSubstitution::index_pair_t &currentPair : currentSubs) {
						if (currentPair.first == indicesCopy[i] && currentPair.second == indices[i]) {
							duplicateSubstitutionIsReverseOfExisting = true;
							break;
						}
					}

					assert(duplicateSubstitutionIsReverseOfExisting);
#endif
				}
			}

			int sign = heapAlg.getParity() ? 1 : -1;

			Terms::IndexSubstitution sub = Terms::IndexSubstitution::createPermutation(std::move(currentSubs), sign);

			// First check whether the given Tensor already has this symmetry and only consider it, if it doesn't
			if (!tensor.getSymmetry().contains(sub)) {
				substitutions.push_back(std::move(sub));
			}
		} while (heapAlg.nextPermutation());

		return substitutions;
	}

protected:
	std::vector< term_t > m_resultingTerms;
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_ANTISYMMETRIZER_HPP_
