#ifndef CONTRACTOR_TERMS_COMPOSITETERM_HPP_
#define CONTRACTOR_TERMS_COMPOSITETERM_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"
#include "terms/TensorSubstitution.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Contractor::Terms {

/**
 * A composite Term is a Term that consists of many (different) additional contributions. Since the Term class
 * itself can't handle explicit representation of additions, these have to be represented as distinct Terms that
 * all share the same result Tensor. However during processing this procedure can easily lead to ambiguity as to
 * which Terms really belong together and which are just identical terms repeated multiple times.
 * Therefore the CompositeTerm class is meant to encapsulate additive contributions to a single result and thus
 * grouping Terms together based on this property thereby eliminating the ambiguity.
 */
template< typename term_t > class CompositeTerm {
public:
	explicit CompositeTerm(const term_t &term) : m_terms({ term }) { checkTerms(); }
	explicit CompositeTerm(term_t &&term) : m_terms({ std::move(term) }) { checkTerms(); }
	explicit CompositeTerm(const std::vector< term_t > &terms) : m_terms(terms) { checkTerms(); }
	explicit CompositeTerm(std::vector< term_t > &&terms = {}) : m_terms(std::move(terms)) { checkTerms(); }

	CompositeTerm(const CompositeTerm< term_t > &) = default;
	CompositeTerm(CompositeTerm< term_t > &&)      = default;
	CompositeTerm< term_t > &operator=(const CompositeTerm< term_t > &) = default;
	CompositeTerm< term_t > &operator=(CompositeTerm< term_t > &&) = default;

	term_t &operator[](std::size_t index) {
		assert(index < m_terms.size());

		return m_terms[index];
	}
	const term_t &operator[](std::size_t index) const {
		assert(index < m_terms.size());

		return m_terms[index];
	}

	std::size_t size() const { return m_terms.size(); }

	auto begin() { return m_terms.begin(); }
	auto end() { return m_terms.end(); }
	auto begin() const { return m_terms.cbegin(); }
	auto end() const { return m_terms.cend(); }
	auto cbegin() const { return m_terms.cbegin(); }
	auto cend() const { return m_terms.cend(); }

	friend bool operator==(const CompositeTerm< term_t > &lhs, const CompositeTerm< term_t > &rhs) {
		return lhs.size() == rhs.size() && std::is_permutation(lhs.begin(), lhs.end(), rhs.begin());
	}
	friend bool operator!=(const CompositeTerm< term_t > &lhs, const CompositeTerm< term_t > &rhs) {
		return !(lhs == rhs);
	}

	friend std::ostream &operator<<(std::ostream &stream, const CompositeTerm< term_t > &composite) {
		stream << "{{ ";
		for (std::size_t i = 0; i < composite.size(); ++i) {
			stream << composite[i];

			if (i + 1 < composite.size()) {
				stream << ", ";
			} else {
				stream << " ";
			}
		}

		stream << "}}";

		return stream;
	}

	const Tensor &getResult() const {
		assert(size() > 0);

		return m_terms[0].getResult();
	}

	void setResult(const Tensor &result) {
		for (term_t &currentTerm : m_terms) {
			currentTerm.setResult(result);
		}
	}

	void addTerm(const term_t &term) {
		checkTerm(term);

		m_terms.push_back(term);
	}

	std::vector< term_t > &accessTerms() { return m_terms; };
	const std::vector< term_t > &getTerms() const { return m_terms; };

	void setTerms(const std::vector< term_t > &terms) {
		m_terms = terms;

		checkTerms();
	}
	void setTerms(std::vector< term_t > &&terms) {
		m_terms = std::move(terms);

		checkTerms();
	}

	void checkTerms() {
		for (const term_t &current : m_terms) {
			checkTerm(current);
		}
	}

	/**
	 * @param other The composite term to check against
	 * @returns Whether this term and the given one are related (differ at most by a factor)
	 */
	bool isRelatedTo(const CompositeTerm< term_t > &other) const { return getRelationFactor(other) != 0; }

	/**
	 * Gets the relation of this term to the given one. The relation is expressed as a TensorSubstitution such
	 * that the result Tensor of this composite can be replaced by the result Tensor of the given composite.
	 * Note: This function assumes that the terms are actually related
	 *
	 * @param other The composite term to check against
	 * @returns The substitution representing the relation
	 */
	TensorSubstitution getRelation(const CompositeTerm< term_t > &other) const {
		Term::factor_t relationFactor = getRelationFactor(other);
		assert(relationFactor != 0);

		return TensorSubstitution(getResult(), other.getResult(), relationFactor);
	}

protected:
	std::vector< term_t > m_terms;

	void checkTerm(term_t term) {
		if (size() != 0 && getResult() != term.getResult()) {
			throw std::runtime_error(
				"Composite Term contains a Term with a result Tensor that doesn't match the other terms");
		}
	}

	Term::factor_t getRelationFactor(const CompositeTerm< term_t > &other) const {
		if (size() != other.size()) {
			return false;
		}

		const Term::term_body_is_same_ignore_factor isRelatedTermBody;

		std::vector< std::reference_wrapper< const term_t > > ownTerms(begin(), end());
		std::vector< std::reference_wrapper< const term_t > > otherTerms(other.begin(), other.end());

		// Make sure the Terms are always in a defined order
		std::sort(ownTerms.begin(), ownTerms.end());
		std::sort(otherTerms.begin(), otherTerms.end());
		// return std::is_permutation(begin(), end(), other.begin(), isRelatedTermBody);

		Term::factor_t relationFactor = 0;
		for (std::size_t i = 0; i < size(); ++i) {
			if (!isRelatedTermBody(ownTerms[i], otherTerms[i])) {
				return 0;
			}

			static_assert(std::is_floating_point_v< Term::factor_t >,
						  "Storing result of division in non-floating point type will cause data loss");
			Term::factor_t currentFactor = ownTerms[i].get().getPrefactor() / otherTerms[i].get().getPrefactor();

			if (relationFactor != 0
				&& std::abs(currentFactor - relationFactor)
					   > std::pow(10, -std::numeric_limits< Term::factor_t >::digits10 + 3)) {
				// The terms themselves might be related but the factors don't yield a consistent relation for all terms
				return 0;
			}

			relationFactor = currentFactor;
		}

		return relationFactor;
	}
};

using BinaryCompositeTerm  = CompositeTerm< BinaryTerm >;
using GeneralCompositeTerm = CompositeTerm< GeneralTerm >;

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_COMPOSITETERM_HPP_
