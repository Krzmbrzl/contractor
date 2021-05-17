#ifndef CONTRACTOR_UTILS_ITERABLE_HPP_
#define CONTRACTOR_UTILS_ITERABLE_HPP_

#include <functional>
#include <iterator>
#include <type_traits>

namespace Contractor {

/*
 * In here we basically define an interface for an object of which we only
 * know/assume that it can be iterated using STL-like iterators.
 * As iterators are always passed by vale in C++, we have to play a few
 * tricks to make this work (creating and accessing a virtual function
 * will most likely lead to "slicing" due to iterators not being used as
 * reference or pointer types).
 *
 * The trick we play is this: We assume that the object that actually is
 * holding the items that we want to iterate over can be asked to access
 * its elements by an index.
 * Then we instantiate our iterable interface object with a function pointer
 * to the function that returns the element corresponding to the given index.
 * By using a function pointer we can achieve our polymorphic requirements
 * indirectly (in the iterable's internals).
 * Furthermore we need to know the start and end of the range we want
 * to iterate over in order to be able to construct the corresponding begin
 * and end iterators.
 * The final ingredient to our iterable interface is a pointer identifying
 * the iterated container. This is needed in order to be able to discriminate
 * iterators that currently point to the same index but are connected to
 * different containers. Thus the pointer is meant to be an identification of
 * the underlaying container (usually just its memory address). This is needed
 * because function pointers (or std::function to be more precise) can't be
 * compared with one another.
 */

template< typename T > struct iterator_impl {
	using iterator_category = std::forward_iterator_tag;
	using difference_type   = std::ptrdiff_t;
	using value_type        = T;
	using pointer           = T *;
	using reference         = T &;

	using access_function_t = std::function< T &(std::size_t) >;

	// regular Ctor
	iterator_impl(std::size_t start, access_function_t &func, const void *id)
		: m_index(start), m_func(func), m_id(id) {}

	// function-move Ctor
	iterator_impl(std::size_t start, access_function_t &&func, const void *id)
		: m_index(start), m_func(func), m_id(id) {}

	// copy Ctor
	iterator_impl(const iterator_impl &) = default;

	// move ctor
	iterator_impl(iterator_impl &&other) {
		std::swap(m_index, other.m_index);
		m_func = std::move(other.m_func);
		std::swap(m_id, other.m_id);
	}

	// copy-assignment
	iterator_impl &operator=(const iterator_impl &other) = default;

	// prefix-increment
	iterator_impl &operator++() {
		++m_index;
		return *this;
	}

	// postfix-increment
	iterator_impl operator++(int) {
		iterator_impl old = *this;
		++(*this);
		return old;
	}

	bool operator==(const iterator_impl &other) const { return m_index == other.m_index && m_id == other.m_id; }

	bool operator!=(const iterator_impl &other) const { return !(*this == other); }

	T &operator*() const { return m_func(m_index); }

	T *operator->() const { return &m_func(m_index); };

protected:
	std::size_t m_index = 0;
	access_function_t m_func;
	const void *m_id = nullptr;
};

template< typename T > struct Iterable {
	using iterator       = iterator_impl< T >;
	using const_iterator = iterator_impl< const std::remove_const_t< T > >;

	Iterable(std::size_t start, std::size_t end, typename iterator_impl< T >::access_function_t &func, const void *id)
		: m_begin(start, func, id), m_end(end, func, id) {}

	iterator begin() { return m_begin; }
	iterator end() { return m_end; }

	const_iterator begin() const { return m_begin; }
	const_iterator end() const { return m_end; }

	const_iterator cbegin() const { return m_begin; }
	const_iterator cend() const { return m_end; }

protected:
	iterator m_begin;
	iterator m_end;
};

}; // namespace Contractor

#endif // CONTRACTOR_UTILS_ITERABLE_HPP_
