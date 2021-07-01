#ifndef CONTRACTOR_UTILS_TERMLIST_HPP_
#define CONTRACTOR_UTILS_TERMLIST_HPP_

#include "terms/Term.hpp"

#include <iterator>
#include <vector>


namespace Contractor::Utils {

/**
 * A non-owning list of Terms that is intended to facilitate further processing by ordering the Terms
 * in such a way that where possible Tensors are first defined via on or more Terms before they are
 * being referenced in a different Term.
 * Bexond that further utility functionalities are implemented such as replacing a Tensor with a different
 * one throughout all Terms.
 */
class TermList {
public:
	/**
	 * The type of the interanally used list
	 */
	using list_t = std::vector< Terms::Term * >;

	TermList() = default;

	/**
	 * Add the range described by the given iterators to this list
	 *
	 * @param it The iterator to the first element to add
	 * @param end The iterator to the element one after the last element to add
	 */
	template< typename iterator_t > void add(iterator_t it, iterator_t end) {
		static_assert(std::is_base_of< Terms::Term, typename std::iterator_traits< iterator_t >::value_type >::value,
					  "Expected iterator yielding a Term");
		while (it != end) {
			m_terms.push_back(&(*it));
			it++;
		}

		sortTerms();
	}

	/**
	 * Add the given Term to this list
	 *
	 * @param term The Term to be added
	 * @param sort Whether the list should be re-sorted after inserting this Term
	 */
	void add(Terms::Term &term, bool sort = true);

	list_t::iterator begin();
	list_t::iterator end();

	list_t::const_iterator begin() const;
	list_t::const_iterator end() const;

	list_t::const_iterator cbegin() const;
	list_t::const_iterator cend() const;

	list_t::reverse_iterator rbegin();
	list_t::reverse_iterator rend();

	list_t::const_reverse_iterator rbegin() const;
	list_t::const_reverse_iterator rend() const;

	list_t::const_reverse_iterator crbegin() const;
	list_t::const_reverse_iterator crend() const;

	Terms::Term &operator[](std::size_t index);

	void clear();

	std::size_t size() const;

	/**
	 * Replace the given Tensor with the provided replacement throughout all Terms contained
	 * in this list.
	 *
	 * @param tensor The Tensor to replace
	 * @param with Its replacement
	 */
	void replace(const Terms::Tensor &tensor, const Terms::Tensor &with);

	/**
	 * Sort the contained Terms into to proper order
	 */
	void sortTerms();

protected:
	std::vector< Terms::Term * > m_terms;
};

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_TERMLIST_HPP_
