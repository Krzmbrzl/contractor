#ifndef CONTRACTOR_PROCESSOR_SPINSUMMATION_HPP_
#define CONTRACTOR_PROCESSOR_SPINSUMMATION_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"
#include "terms/TensorDecomposition.hpp"

#include <algorithm>
#include <bitset>
#include <unordered_set>
#include <vector>

namespace Contractor::Processor::SpinSummation {

namespace details {

	/**
	 * @returns Whether the given Index is of Type None
	 */
	bool is_NoneType(const Terms::Index &index) { return index.getType() == Terms::Index::Type::None; }

	/**
	 * @returns An IndexSubstitution with the given sign that will map the given indices to themselves except that all
	 * of them will now have Spin None
	 */
	Terms::IndexSubstitution mapToSpinFreeIndices(const Terms::Tensor::index_list_t &indices, int sign = 1) {
		Terms::IndexSubstitution::substitution_list substitutions;

		for (const Terms::Index &currentIndex : indices) {
			// We expect that there are no indices with spin "Both" at this point. Furthermore we expect creator and
			// annihilator indices to have either spin Alpha or Beta whereas all other indices are expected to have spin
			// None
			assert(currentIndex.getSpin() != Terms::Index::Spin::Both);
			assert(currentIndex.getType() == Terms::Index::Type::None
				   || currentIndex.getSpin() != Terms::Index::Spin::None);
			assert(currentIndex.getType() != Terms::Index::Type::None
				   || currentIndex.getSpin() == Terms::Index::Spin::None);

			Terms::Index replacement = currentIndex;
			replacement.setSpin(Terms::Index::Spin::None);

			substitutions.push_back({ currentIndex, std::move(replacement) });
		}

		return Terms::IndexSubstitution(std::move(substitutions), sign);
	}

	/**
	 * @returns A TensorDecomposition that will replace the given Tensor with a sum of Tensors that result from applying
	 * the given replacement substitutions to the original Tensor.
	 */
	Terms::TensorDecomposition replaceTensorWith(const Terms::Tensor &tensor,
												 const std::vector< Terms::IndexSubstitution > &replacements) {
		Terms::TensorDecomposition::substitution_list_t substitutions;

		for (const Terms::IndexSubstitution &currentSub : replacements) {
			assert(currentSub.appliesTo(tensor));

			Terms::Tensor replacement                 = tensor;
			Terms::IndexSubstitution::factor_t factor = currentSub.apply(replacement);

			substitutions.push_back(Terms::GeneralTerm(tensor, factor, { std::move(replacement) }));
		}

		return Terms::TensorDecomposition(std::move(substitutions));
	}

	/**
	 * @returns Whether all creator and all annihilator indices have the same index space
	 * respectively
	 */
	bool indexGroupsAreSameSpace(const Terms::Tensor &tensor) {
		if (tensor.getIndices().empty()) {
			return true;
		}

		Terms::IndexSpace space = tensor.getIndices()[0].getSpace();
		Terms::Index::Type type = tensor.getIndices()[0].getType();
		for (std::size_t i = 1; i < tensor.getIndices().size(); ++i) {
			if (tensor.getIndices()[i].getType() == type) {
				if (tensor.getIndices()[i].getSpace() != space) {
					return false;
				}
			} else {
				space = tensor.getIndices()[i].getSpace();
				type  = tensor.getIndices()[i].getType();

				if (type == Terms::Index::Type::None) {
					// We don't care about non-creators and -annihilators
					break;
				}
			}
		}

		return true;
	}

	/**
	 * @returns The amount of indices in the given index list that are of the given type
	 */
	unsigned int countIndexType(const Terms::Tensor::index_list_t &indices, Terms::Index::Type type) {
		unsigned int count   = 0;
		bool foundTypeBefore = false;

		for (const Terms::Index &currentIndex : indices) {
			if (currentIndex.getType() == type) {
				count++;
				foundTypeBefore = true;
			} else if (foundTypeBefore) {
				// The indices are grouped by type so if we exceed the group of the searched type, we
				// can quit our search right here
				return count;
			}
		}

		return count;
	}

	/**
	 * @returns (One of) the antisymmetry of the given tensor
	 */
	Terms::IndexSubstitution findAntisymmetry(const Terms::Tensor &tensor) {
		assert(tensor.getIndices().size() >= 4);
		assert(tensor.isPartiallyAntisymmetrized());

		Terms::IndexSubstitution sym =
			Terms::IndexSubstitution::createPermutation({ { tensor.getIndices()[0], tensor.getIndices()[1] } }, -1);

		if (!tensor.getSymmetry().contains(sym)) {
			sym =
				Terms::IndexSubstitution::createPermutation({ { tensor.getIndices()[2], tensor.getIndices()[3] } }, -1);
		}

		assert(tensor.getSymmetry().contains(sym));

		return sym;
	}

	/**
	 * Creates a mapping from the given Tensor to its corresponding skeleton Tensor.
	 *
	 * @param tensor The tensor to map from
	 * @param sign The sign of the desired mapping
	 * @param antisymmetrize Whether the mapped Tensor is to be antisymmetrized
	 * @returns A TensorDecomposition that describes the decomposition of the given tensor into the desired mapping
	 */
	Terms::TensorDecomposition mapToSkeletonTensor(const Terms::Tensor &tensor, int sign, bool antisymmetrize) {
		assert(tensor.getIndices().size() >= 4);

		Terms::IndexSubstitution spinFreeMapping = mapToSpinFreeIndices(tensor.getIndices(), sign);

		Terms::TensorDecomposition::substitution_list_t substitutions;

		Terms::Tensor replacement                 = tensor;
		Terms::IndexSubstitution::factor_t factor = spinFreeMapping.apply(replacement);

		substitutions.push_back(Terms::GeneralTerm(tensor, factor, { std::move(replacement) }));

		if (antisymmetrize) {
			Terms::IndexSubstitution antisymmetrization = findAntisymmetry(tensor);

			replacement = tensor;
			// First apply the antisymmetrization and then map to spin-free indices
			factor = (spinFreeMapping * antisymmetrization).apply(replacement);

			substitutions.push_back(Terms::GeneralTerm(tensor, factor, { std::move(replacement) }));
		}

		bool originalTensorIsFullyAntisymmetric = tensor.isAntisymmetrized();

		// Ensure proper symmetry of the skeleton Tensor(s)
		for (Terms::GeneralTerm &current : substitutions) {
			Terms::Tensor &currentTensor = current.accessTensorList()[0];

			Terms::PermutationGroup symmetry(currentTensor.getIndices());

			if (originalTensorIsFullyAntisymmetric) {
				// The skeleton Tensor only has column symmetry if the original Tensor is fully antisymmetric. Otherwise
				// the skeleton Tensor does not show any symmetry.
				Terms::IndexSubstitution columnSymmetry = Terms::IndexSubstitution::createPermutation(
					{ { currentTensor.getIndices()[0], currentTensor.getIndices()[1] },
					  { currentTensor.getIndices()[2], currentTensor.getIndices()[3] } });

				symmetry.addGenerator(columnSymmetry);
			}

			currentTensor.setSymmetry(symmetry);
		}

		return Terms::TensorDecomposition(std::move(substitutions));
	}

	using spin_bitset = std::bitset< 8 >;

	/**
	 * @returns A bitset representing the given tensor's spin case. 1 means Beta and 0 means Alpha.
	 */
	spin_bitset determineSpinCase(const Terms::Tensor &tensor) {
		// Create a bit-pattern that describes the spin-case of the input (1 bits means beta, 0 bits means alpha)
		spin_bitset spinCase;
		assert(spinCase.size() >= tensor.getIndices().size());

		for (int i = 0; i < tensor.getIndices().size(); ++i) {
			if (tensor.getIndices()[i].getSpin() == Terms::Index::Spin::Beta) {
				spinCase.set(i);
			} else if (tensor.getIndices()[i].getType() == Terms::Index::Type::None) {
				// We expect that there won't come any indices associated with a spin after
				// this because indices are sorted based on their type
				break;
			}
		}

		return spinCase;
	}

	/**
	 * @returns The amount of creator and annihilator indices in the given Tensor
	 */
	std::size_t getRelevantIndexCount(const Terms::Tensor &tensor) {
		// We assume that the indices in this tensor are order such that creators and annihilators
		// come before additional indices. For our purposes everything except creator and annihilator
		// indices are irrelevant since these other indices are expected to carry no spin anyway.
		auto indexEnd = std::find_if(tensor.getIndices().begin(), tensor.getIndices().end(), is_NoneType);

		return std::distance(tensor.getIndices().begin(), indexEnd);
	}

	/**
	 * @returns A TensorDecomposition describing the result of processing the given Tensor
	 */
	Terms::TensorDecomposition processTensor(const Terms::Tensor &tensor) {
		std::size_t relevantIndexCount = getRelevantIndexCount(tensor);

		if (relevantIndexCount % 2 != 0) {
			// Only allow even number of (relevant) indices
			throw std::runtime_error("Can't spin-sum a Tensor with an uneven amount of (relevant) indices");
		}

		spin_bitset spinCase = determineSpinCase(tensor);

		const spin_bitset allBeta = (1 << relevantIndexCount) - 1;
		const spin_bitset allAlpha;

		if (relevantIndexCount == 0) {
			// Nothing to do for this Tensor
			return {};
		}
		if (relevantIndexCount == 2) {
			if (spinCase != allAlpha && spinCase != allBeta) {
				// Mixed-spin cases should not occur for 2-index Tensors
				throw std::runtime_error("Invalid spin-case for 2-index Tensor encountered during spin-summation");
			}

			// In the 2-index case it doesn't matter whether the result is all-alpha or all-beta as both are the same.
			// Thus we might as well forget about the explicit spin of the indices and instead convert the indices to
			// spin-free ones in both cases (turning both cases into equal expressions).
			return replaceTensorWith(tensor, { mapToSpinFreeIndices(tensor.getIndices()) });
		}
		if (relevantIndexCount == 4) {
			if (!tensor.isPartiallyAntisymmetrized()) {
				// Without this property we can't map the Tensor to a skeleton Tensor (without further consideration)
				std::cerr << tensor << std::endl;
				throw std::runtime_error(
					"Unable to spin-sum a 4-index Tensor that is not at least partially antisymmetric");
			}
			if (!indexGroupsAreSameSpace(tensor)) {
				throw std::runtime_error("Unsupported case encountered in spin-summation (creator and/or annihilator "
										 "contain indices of different index spaces)");
			}
			if (countIndexType(tensor.getIndices(), Terms::Index::Type::Creator) != 2) {
				throw std::runtime_error("Expected 4-index Tensor to have 2 creator and 2 annihilator indices");
			}

			// Until here we have verified that we have a Tensor that has 4 relevant indices of which 2 are creator
			// indices (and thus the other 2 are annihilator indices). Furthermore we have ensured that both creators
			// and both annihilators refer to the same index space. And finally we know that the Tensor is at least
			// antisymmetric with regards to exchange of the two creator or the two annihilator indices. Under these
			// preconditions we can map all mixed-spin cases to a single "skeleton Tensor" that is the same (except for
			// the sign) for all mixed-spin cases. Thus in order to arrive at the skeleton Tensor we only map the
			// indices to spin-free ones and account for the sign. The all-alpha and all-beta cases are the same
			// (spin-reversal symmetry) and can be mapped to these skeleton Tensors by antisymmetrization: t[ab,ij] =
			// T[ab,ij] - T[ba,ij] It is essential for only partial antisymmetric Tensors to perform this
			// antisymmetrization over the index pair that is also antisymmetric in the original Tensor. For fully
			// antisymmetric Tensors the choice over which index pair is antisymmetrized doesn't matter.
			//
			// Note that if the original Tensor is fully antisymmetric, the skeleton Tensors produced in this way have
			// only particle-1,2-symmetry (column-symmetry). If the original Tensor is only partially antisymmetric,
			// the skeleton Tensor does not show any symmetry.
			int sign            = 1;
			bool antisymmetrize = false;
			switch (spinCase.to_ullong()) {
				case 0b0000:
				case 0b1111:
					// all-alpha or all-beta
					antisymmetrize = true;
					break;
				case 0b1001:
				case 0b0110:
					sign = -1;
					// mixed-spin case that will be mapped to the negative skeleton Tensor
					// Fallthrough
				case 0b0101:
				case 0b1010:
					// mixed-spin case that will be mapped to the skeleton Tensor as-is
					break;
				default:
					throw std::runtime_error("Encountered unexpected spin-case during spin summation");
			}

			return mapToSkeletonTensor(tensor, sign, antisymmetrize);
		}

		// We currently don't support Tensors with more that 4 indices
		throw std::runtime_error("Ran into unimplemented code part during spin summation");
	}

	/**
	 * @returns Whether the given spinCase is consider to be a "canonical spin case"
	 */
	bool isCanonicalSpinCase(spin_bitset spinCase, std::size_t relevantIndexCount) {
		std::size_t nBetaSpins  = spinCase.count();
		std::size_t nAlphaSpins = relevantIndexCount - nBetaSpins;

		if (nBetaSpins > nAlphaSpins) {
			// More Beta than Alpha spins
			return false;
		} else if (nBetaSpins == nAlphaSpins && spinCase.test(0)) {
			// Equal amount of Alpha and Beta spins but the first spin is Beta instead
			// of Alpha
			return false;
		} else {
			return true;
		}
	}

	/**
	 * This function will map the given Tensor to a "canonical spin case" by use of spin-inversion-symmetry
	 * where necessary. A "canonical spin case" is one that either contains more Alpha than Beta spins or
	 * that has an equal amount of them but the first index has spin Alpha.
	 *
	 * @param tensor The Tensor to bring into "canonical spin case"
	 */
	void mapToCanonicalSpinCase(Terms::Tensor &tensor) {
		spin_bitset spinCase = determineSpinCase(tensor);

		std::size_t relevantIndexCount = getRelevantIndexCount(tensor);

		if (isCanonicalSpinCase(spinCase, relevantIndexCount)) {
			return;
		}

		// Invert spins
		spinCase.flip();

		// Assemble the index substitution that performs the spin inversion. We use an IndexSubstitution
		// object instead of manually flipping spins in order to make sure the Tensor's symmetry is
		// adapted accordingly.
		Terms::IndexSubstitution::substitution_list spinFlipMapping;
		for (std::size_t i = 0; i < relevantIndexCount; ++i) {
			Terms::Index replacement = tensor.getIndices()[i];

			replacement.setSpin(spinCase.test(i) ? Terms::Index::Spin::Beta : Terms::Index::Spin::Alpha);

			spinFlipMapping.push_back({ tensor.getIndices()[i], std::move(replacement) });
		}

		// Apply the spin-flip
		Terms::IndexSubstitution(std::move(spinFlipMapping)).apply(tensor);
	}

}; // namespace details

/**
 * Performs spin-summation of the given terms (assuming restricted orbitals - that is the spatial part of alpha and beta
 * electrons are always identical).
 *
 * @param terms A list of Terms to spin-sum. All these terms are assumed to be spin-integrated already.
 * @param nonIntermediateNames A set of names of Tensors which are NOT intermediate Tensors. These are the ones that
 * will be mapped to skeleton Tensors by this function (and which are expected to be at least partially antisymmetric)
 */
template< typename term_t >
std::vector< term_t > sum(const std::vector< term_t > &terms,
						  const std::unordered_set< std::string_view > &nonIntermediateNames) {
	std::vector< term_t > summedTerms;

	for (term_t currentTerm : terms) {
		if (!details::isCanonicalSpinCase(details::determineSpinCase(currentTerm.getResult()),
										  details::getRelevantIndexCount(currentTerm.getResult()))) {
			// We can sort out non-canonical spin cases of the result as we make the assumption that the list of
			// Terms given to us contains all spin cases of any given Term and thus this list must also hold a
			// Term for the canonical spin case of this result, which is all that we need for further processing.
			continue;
		}

		bool isIntermediate =
			nonIntermediateNames.find(currentTerm.getResult().getName()) == nonIntermediateNames.end();

		if (!isIntermediate) {
			// For now we don't want to spin-sum intermediates
			Terms::TensorDecomposition decomposition = details::processTensor(currentTerm.getResult());

			if (decomposition.getSubstitutions().size() > 1) {
				// This result will be expressed as a linear combination of other Tensors. Therefore we don't have
				// calculate it explicitly, meaning that the current term is superfluous.
				continue;
			}
			if (decomposition.isValid()) {
				// A Tensor decomposition is meant to work on the rhs of a Term, so we have to cheat a bit in order
				// to get it to work on the result Tensor.
				Terms::GeneralTerm dummyTerm(Terms::Tensor(), 1, { currentTerm.getResult() });

				bool successful                         = false;
				std::vector< Terms::GeneralTerm > terms = decomposition.apply(dummyTerm, &successful);

				assert(successful);
				assert(terms.size() == 1);
				assert(terms[0].accessTensorList().size() == 1);

				// Overwrite the result Tensor of the current result with the result of the decomposition
				currentTerm.accessResult() = std::move(terms[0].accessTensorList()[0]);

				// The factor of the dummy term represents the sign of the mapping
				currentTerm.setPrefactor(currentTerm.getPrefactor() * terms[0].getPrefactor());
			}
		}

		// Process all Tensors and see how they are to be decomposed
		std::vector< Terms::TensorDecomposition > decompositions;
		for (Terms::Tensor &currentTensor : currentTerm.accessTensors()) {
			bool isIntermediate = nonIntermediateNames.find(currentTensor.getName()) == nonIntermediateNames.end();

			if (isIntermediate) {
				// Map to canonical spin case
				details::mapToCanonicalSpinCase(currentTensor);

				// For now we don't want to map intermediate Tensors to skeleton Tensors, so we can end
				// the current iteration here.
				continue;
			}

			Terms::TensorDecomposition currentDecomposition = details::processTensor(currentTensor);

			if (currentDecomposition.isValid()) {
				decompositions.push_back(std::move(currentDecomposition));
			}
		}

		if (decompositions.empty()) {
			summedTerms.push_back(std::move(currentTerm));
		} else {
			// Apply the given decompositions to the current Term
			bool successful                                        = false;
			Terms::TensorDecomposition::decomposed_terms_t results = decompositions[0].apply(currentTerm, &successful);

			assert(successful);

			for (std::size_t i = 1; i < decompositions.size(); ++i) {
				Terms::TensorDecomposition::decomposed_terms_t newResults;

				for (const Terms::GeneralTerm &current : results) {
					Terms::TensorDecomposition::decomposed_terms_t currentResults =
						decompositions[i].apply(current, &successful);

					assert(successful);

					newResults.insert(newResults.end(), std::make_move_iterator(currentResults.begin()),
									  std::make_move_iterator(currentResults.end()));
				}

				results = std::move(newResults);
			}

			// Add the resulting Terms to summedTerms
			for (Terms::GeneralTerm &current : results) {
				if constexpr (std::is_same_v< Terms::BinaryTerm, term_t >) {
					// Convert to binary Terms (under the assumption that each term does not contain > 2 elements
					summedTerms.push_back(Terms::BinaryTerm::toBinaryTerm(current));
				} else {
					summedTerms.push_back(std::move(current));
				}
			}
		}
	}

	return summedTerms;
}

}; // namespace Contractor::Processor::SpinSummation

#endif // CONTRACTOR_PROCESSOR_SPINSUMMATION_HPP_
