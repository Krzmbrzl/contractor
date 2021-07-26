#ifndef CONTRACTOR_PROCESSOR_SYMMETRIZER_HPP_
#define CONTRACTOR_PROCESSOR_SYMMETRIZER_HPP_

#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "terms/Term.hpp"
#include "utils/HeapsAlgorithm.hpp"

#include <cassert>
#include <numeric>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <boost/range/join.hpp>

namespace Contractor::Processor {

namespace details {
	template< Terms::Index::Type Type > struct is_index_type {
		bool operator()(const Terms::Index &index) const { return index.getType() == Type; }
	};
}; // namespace details

template< typename term_t > class Symmetrizer {
public:
	Symmetrizer()  = default;
	~Symmetrizer() = default;

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

	const std::vector< term_t > &symmetrize(const term_t &term, bool ignoreExistingSymmetries = false) {
		m_resultingTerms.clear();

		std::vector< Terms::IndexSubstitution > symmetrizations =
			generateSymmetrizations(term, ignoreExistingSymmetries);

		// We expect at least the identity operation to always be present
		assert(!symmetrizations.empty());

		// Copy Terms so we can access it
		term_t termCopy = term;

		for (const Terms::IndexSubstitution &currentSymmetry : symmetrizations) {
			// "Inform" the result Tensor about its (potentially) new symmetry
			termCopy.accessResult().accessSymmetry().addGenerator(currentSymmetry);
		}

		for (const Terms::IndexSubstitution &currentSymmetrization : symmetrizations) {
			assert(currentSymmetrization.appliesTo(termCopy.getResult()));

			term_t currentCopy = termCopy;

			Terms::IndexSubstitution::factor_t factor = 1;

			// Also apply to all contained Tensors
			for (Terms::Tensor &currenTensor : currentCopy.accessTensors()) {
				factor = currentSymmetrization.apply(currenTensor);
			}

			termCopy.setPrefactor(currentCopy.getPrefactor() * factor);

			m_resultingTerms.push_back(std::move(currentCopy));
		}

		return m_resultingTerms;
	}

	std::vector< Terms::IndexSubstitution > generateSymmetrizations(const term_t &term, bool ignoreExistingSymmetries) {
		std::vector< Terms::IndexSubstitution > symmetrizations;
		// The identity operation is always contained in order to retain the original term as-is
		symmetrizations.push_back(Terms::IndexSubstitution::identity());

		// Gather information about index distribution
		// We assume indices are ordered as Creators, Annihilators, Other
		const Terms::Tensor::index_list_t &indices = term.getResult().getIndices();
		auto creatorBegin                          = indices.begin();
		auto annihilatorBegin =
			std::find_if(creatorBegin, indices.end(), details::is_index_type< Terms::Index::Type::Annihilator >{});
		auto annihilatorEnd =
			std::find_if(annihilatorBegin, indices.end(), details::is_index_type< Terms::Index::Type::None >{});
		auto creatorEnd = annihilatorBegin;

		std::size_t nCreators     = std::distance(creatorBegin, creatorEnd);
		std::size_t nAnnihilators = std::distance(annihilatorBegin, annihilatorEnd);

		if (nCreators != nAnnihilators) {
			throw std::runtime_error("Can't symmetrize Tensor with different amounts of creators and annihilators!");
		}

		// Symmetrization works by arranging all creator/annihilator pairs in all possible orders
		std::vector< std::size_t > pairPositions;
		pairPositions.resize(nCreators);
		std::iota(pairPositions.begin(), pairPositions.end(), 0);

		// We always start with the first permutation of pairPositions in order to avoid the case where all
		// index positions are mapped to themselves (identity operation) as that is already explicitly added
		// above.
		while (std::next_permutation(pairPositions.begin(), pairPositions.end())) {
			Terms::IndexSubstitution::substitution_list subs;
			subs.reserve(nCreators);

			for (std::size_t i = 0; i < nCreators; ++i) {
				// Swap the i-th creator/annihilator with the pairPositions[i]-th one such that
				// the pair that is at position i in the original result Tensor is moved to the
				// pairPositions[i]-th position after the substitution.
				subs.push_back({ *(creatorBegin + i), *(creatorBegin + pairPositions[i]) });
				subs.push_back({ *(annihilatorBegin + i), *(annihilatorBegin + pairPositions[i]) });
			}

			// This symmetrization is performed without change in sign
			Terms::IndexSubstitution currentSubstitution(std::move(subs));

			assert(!currentSubstitution.isIdentity());

			if (!ignoreExistingSymmetries && term.getResult().getSymmetry().contains(currentSubstitution)) {
				// This symmetry is already contained, so we don't want to explicitly symmetrize over it again because
				// we were NOT instructed to ignore existing symmetries
				continue;
			}

			symmetrizations.push_back(std::move(currentSubstitution));
		}

		return symmetrizations;
	}

protected:
	std::vector< term_t > m_resultingTerms;
};

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_SYMMETRIZER_HPP_
