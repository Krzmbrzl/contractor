#ifndef CONTRACTOR_TERMS_TERMGROUP_HPP_
#define CONTRACTOR_TERMS_TERMGROUP_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/CompositeTerm.hpp"
#include "terms/GeneralTerm.hpp"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <vector>

namespace Contractor::Terms {

/**
 * A class meant to be used to group Terms together. Every group is associated with an "original Term" which
 * is the one all these Terms belong to (usually the one that was initially read in and that produced the contained
 * Terms by processing of the original Term).
 */
template< typename term_t > class TermGroup {
public:
	using value_type = CompositeTerm< term_t >;

	/**
	 * @param term The Term to create the group from
	 * @returns A group that has the given term as the original Term and also as its only contained Term
	 */
	static TermGroup< term_t > from(const term_t &term) {
		auto group = TermGroup< term_t >(GeneralTerm(term));

		group.addTerm(CompositeTerm< term_t >(term));

		return group;
	}

	TermGroup(const GeneralTerm &originalTerm) { setOriginalTerm(originalTerm); }
	TermGroup(GeneralTerm &&originalTerm) { setOriginalTerm(std::move(originalTerm)); }

	TermGroup(const TermGroup< term_t > &) = default;
	TermGroup(TermGroup< term_t > &&)      = default;
	TermGroup< term_t > &operator=(const TermGroup< term_t > &) = default;
	TermGroup< term_t > &operator=(TermGroup< term_t > &&) = default;

	const std::vector< value_type > &getTerms() const { return m_terms; }
	std::vector< value_type > &accessTerms() { return m_terms; }
	void setTerms(const std::vector< value_type > &terms) { m_terms = terms; }
	void setTerms(std::vector< value_type > &&terms) { m_terms = std::move(terms); }
	void addTerm(const value_type &term) { m_terms.push_back(term); }
	void addTerm(value_type &&term) { m_terms.push_back(std::move(term)); }
	void addTerm(const term_t &term) { addTerm(CompositeTerm< term_t >(term)); }
	void addTerm(term_t &&term) { addTerm(CompositeTerm< term_t >(std::move(term))); }

	const GeneralTerm &getOriginalTerm() const { return m_originalTerm; }
	void setOriginalTerm(const GeneralTerm &term) { m_originalTerm = term; }
	void setOriginalTerm(GeneralTerm &&term) { m_originalTerm = std::move(term); }

	std::size_t size() const { return m_terms.size(); }

	value_type &operator[](std::size_t index) {
		assert(index < m_terms.size());
		return m_terms[index];
	}
	const value_type &operator[](std::size_t index) const {
		assert(index < m_terms.size());
		return m_terms[index];
	}

	friend bool operator==(const TermGroup< term_t > &lhs, const TermGroup< term_t > &rhs) {
		return lhs.size() == rhs.size() && lhs.getOriginalTerm() == rhs.getOriginalTerm()
			   && std::is_permutation(lhs.m_terms.begin(), lhs.m_terms.end(), rhs.m_terms.begin());
	}
	friend bool operator!=(const TermGroup< term_t > &lhs, const TermGroup< term_t > &rhs) { return !(lhs == rhs); }

	friend std::ostream &operator<<(std::ostream &stream, const TermGroup< term_t > &container) {
		stream << "[\nOriginal: " << container.getOriginalTerm() << "\n";

		for (std::size_t i = 0; i < container.size(); ++i) {
			stream << " > " << container[i];

			if (i + 1 < container.size()) {
				stream << std::endl;
			}
		}

		return stream;
	}

	auto begin() { return m_terms.begin(); }
	auto end() { return m_terms.end(); }
	auto begin() const { return m_terms.cbegin(); }
	auto end() const { return m_terms.cend(); }
	auto cbegin() const { return m_terms.cbegin(); }
	auto cend() const { return m_terms.cend(); }

protected:
	GeneralTerm m_originalTerm;
	std::vector< value_type > m_terms;
};

using BinaryTermGroup  = TermGroup< BinaryTerm >;
using GeneralTermGroup = TermGroup< GeneralTerm >;

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TERMGROUP_HPP_
