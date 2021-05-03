#ifndef CONTRACTOR_UTILS_ITERABLEVIEW_HPP_
#define CONTRACTOR_UTILS_ITERABLEVIEW_HPP_

#include <iterator>
#include <type_traits>

namespace Contractor {

/**
 * A light-weight, non-owning wrapper around any iterable object. This is meant to be returned
 * instead of the container wrapped object if only iterability is to be exposed to the outside.
 */
template< typename DataType > class IterableView {
public:
	IterableView(std::remove_reference_t< DataType > &wrappedData) : m_wrappedData(wrappedData) {}

	typename DataType::iterator begin() { return std::begin(m_wrappedData); }
	typename DataType::iterator end() { return std::end(m_wrappedData); }

	typename DataType::const_iterator begin() const { return std::begin(m_wrappedData); }
	typename DataType::const_iterator end() const { return std::end(m_wrappedData); }

	typename DataType::const_iterator cbegin() const { return std::cbegin(m_wrappedData); }
	typename DataType::const_iterator cend() const { return std::cend(m_wrappedData); }

	std::size_t size() const { return std::distance(begin(), end()); };

protected:
	std::remove_reference_t< DataType > &m_wrappedData;
};

}; // namespace Contractor

#endif // CONTRACTOR_UTILS_ITERABLEVIEW_HPP_
