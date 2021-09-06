#ifndef CONTRACTOR_PROCESSOR_SIMPLIFIER_HPP_
#define CONTRACTOR_PROCESSOR_SIMPLIFIER_HPP_

#include "processor/PrinterWrapper.hpp"
#include "terms/CompositeTerm.hpp"
#include "terms/TensorSubstitution.hpp"
#include "terms/Term.hpp"
#include "terms/TermGroup.hpp"
#include "utils/SortUtils.hpp"

#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <type_traits>
#include <vector>

namespace Contractor::Processor {

namespace details {
	/**
	 * Custom adaption of the implementaton of std::unique which apart from performing the job of
	 * std::unique also calls the given function every time a duplicate element is encountered.
	 */
	template< typename ForwardIt, typename BinaryPredicate >
	ForwardIt unique_term(ForwardIt first, ForwardIt last, BinaryPredicate pred,
						  const std::function< void(Terms::Term &, Terms::Term &) > &func) {
		static_assert(std::is_base_of_v< Terms::Term, typename ForwardIt::value_type >,
					  "Expected iterator that resolves to a Term");

		if (first == last) {
			// There are no elements in here
			return last;
		}

		ForwardIt result = first;

		// then increment first and check if we are done
		while (++first != last) {
			// if the value of first is still the same as the value of result
			// then restart the loop (incrementing first and checking if we are done)
			// Notice that result isn't moved until the values differ
			if (pred(*result, *first)) {
				// Call the given function with the duplicate
				func(*result, *first);

				continue;
			}

			// increment result and move the value of first to this new spot
			// as long as they don't point to the same place
			// So result is only moved when first points to a new (different) value
			if (++result != first) {
				*result = std::move(*first);
			}
		}

		// return one past the end of the new (possibly shorter) range.
		return ++result;
	}

	struct compatible_term {
		bool operator()(const Terms::Term &lhs, const Terms::Term &rhs) const {
			// Terms are considered to be "compatible" if they are equal up to the prefactor
			return lhs.getResult() == rhs.getResult()
				   && std::equal(lhs.getTensors().begin(), lhs.getTensors().end(), rhs.getTensors().begin(),
								 rhs.getTensors().end());
		}
	};

	static void sumCompatible(Terms::Term &original, Terms::Term &duplicate) {
		original.setPrefactor(original.getPrefactor() + duplicate.getPrefactor());
	}
}; // namespace details

bool canonicalizeIndexIDs(Terms::Term &term);

bool canonicalizeIndexSequences(Terms::Term &term);

template< typename term_t >
bool simplify(std::vector< term_t > &terms, bool independentTerms = true, PrinterWrapper printer = {}) {
	bool changed = false;
	static_assert(std::is_base_of_v< Terms::Term, term_t >, "Simplify function can only be called on vector of Terms");

	// Start by bringing all Terms into "canonical representation".
	for (term_t &currentTerm : terms) {
		term_t originalTerm = currentTerm;

		bool currentChanged = false;
		std::stringstream simplifications;

		if (canonicalizeIndexSequences(currentTerm)) {
			// The first reorder makes sure that we arrive at a deterministic spin case for
			// our Tensors
			simplifications << "reorder;";
			currentChanged = true;
		}

		if (canonicalizeIndexIDs(currentTerm)) {
			simplifications << "rename;";
			currentChanged = true;
		}

		if (canonicalizeIndexSequences(currentTerm)) {
			// The second reorder takes care of "prettifying" the Tensors after indices with
			// same spin have been renamed
			simplifications << "reorder;";
			currentChanged = true;
		}

		if (currentChanged) {
			printer << "Term " << originalTerm << " simplifies to\n     " << currentTerm
					<< " using these index operations: " << simplifications.str() << "\n";

			changed = true;
		}
	}

	// sort terms so that equal terms end up next to one another
	std::sort(terms.begin(), terms.end());

	std::size_t originalAmountOfTerms = terms.size();

	if (independentTerms) {
		// If the Terms are independent from one another, we can start simplify by deleting
		// exact duplicates
		terms.erase(std::unique(terms.begin(), terms.end()), terms.end());

		for (const auto &current : terms) {
			std::cout << current << std::endl;
		}

		// For independent terms we expect that there are no Terms which contribute to differ only in their
		// prefactor
		assert(std::unique(terms.begin(), terms.end(), details::compatible_term{}) == terms.end());
	} else {
		// All terms that are equal (except for the factor) can be added up and then we only
		// need a single term with the added prefactor
		terms.erase(
			details::unique_term(terms.begin(), terms.end(), details::compatible_term{}, &details::sumCompatible),
			terms.end());
	}

	if (originalAmountOfTerms != terms.size()) {
		printer << "Out of " << originalAmountOfTerms << " terms " << (originalAmountOfTerms - terms.size())
				<< " were redundant and have been removed\n";
		printer << "The remaining terms are\n" << terms << "\n";

		changed = true;
	}

	return changed;
}

template< typename term_t >
bool simplify(std::vector< Terms::CompositeTerm< term_t > > &composites, PrinterWrapper printer = {}) {
	bool changed = false;

	// In a first step we want to simplify each composite as far as possible
	for (Terms::CompositeTerm< term_t > &currentComposite : composites) {
		if (simplify(currentComposite.accessTerms(), false, printer)) {
			changed = true;
		}
	}

	// Find composite Terms that can be expressed in terms of another one and thus are not needed
	// Note that the algorithm as implemented here does not guarantee that the elements which are
	// not removed will remain in the same order as they had in the original vector. Therefore we
	// will keep track of an element's original position explicitly in order to restore this order
	// again once we are done.
	auto outerIt = composites.begin();
	auto end     = composites.end();

	std::vector< std::size_t > originalPositions(composites.size());
	std::iota(originalPositions.begin(), originalPositions.end(), 0);

	auto outerPosIt = originalPositions.begin();
	auto posEnd     = originalPositions.end();

	std::vector< Terms::TensorSubstitution > substitutions;
	for (auto outerIt = composites.begin(); outerIt != end; ++outerIt) {
		auto innerIt    = outerIt + 1;
		auto innerPosIt = outerPosIt + 1;

		while (innerIt != end) {
			if (innerIt->isRelatedTo(*outerIt)) {
				// These Terms are related. This means that the result of one can be expressed by the result of the
				// other times a factor. Therefore we'll discard the inner Term and only keep the outer one. Also we'll
				// have to remember what this relation is, so that we'll be able to perform the correct adjustments.
				//
				// "Discarding" in this context means writing this term to the end of the list and then pretending
				// that the list really is one element shorter
				if (*outerIt != *innerIt) {
					// We only bother substituting if there actually is a difference in these two Tensors. If they
					// are the same already, we can simply discard the second one.
					Terms::TensorSubstitution sub = innerIt->getRelation(*outerIt);

					printer << "Found a relation such that " << sub << "\n";

					substitutions.push_back(std::move(sub));
				} else {
					printer << "Eliminated duplicate of " << *outerIt << "\n";
				}

				end--;
				posEnd--;
				std::swap(*innerIt, *end);
				std::swap(*innerPosIt, *posEnd);

				changed = true;
			} else {
				innerIt++;
				innerPosIt++;
			}
		}

		outerPosIt++;
	}

	// Actually get rid of the terms that are no longer needed
	composites.erase(end, composites.end());
	originalPositions.erase(posEnd, originalPositions.end());

	// Restore original order between the remaining elements
	Utils::sort_by(composites, originalPositions);

	// Substitute the Tensors whose terms we have discarded
	if (!substitutions.empty()) {
		for (Terms::CompositeTerm< term_t > &currentComposite : composites) {
			for (Terms::Term &currentTerm : currentComposite) {
				for (const Terms::TensorSubstitution &currentSub : substitutions) {
					currentSub.apply(currentTerm, false);
				}
			}
		}

		// As long as we performed some substitutions, it could be that by doing this we
		// allowed for further simplications to be performed.
		simplify(composites, printer);
	}

	return changed;
}

template< typename term_t >
bool simplify(std::vector< Terms::TermGroup< term_t > > &groups, PrinterWrapper printer = {}) {
	bool changed = false;

	for (Terms::TermGroup< term_t > &current : groups) {
		if (simplify(current.accessTerms(), printer)) {
			changed = true;
		}
	}

	// We don't expect groups to be identical and thus we don't check for that
	return changed;
}

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_SIMPLIFIER_HPP_
