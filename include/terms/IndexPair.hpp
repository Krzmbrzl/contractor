#ifndef CONTRACTOR_TERMS_INDEXPAIR_HPP_
#define CONTRACTOR_TERMS_INDEXPAIR_HPP_

#include "terms/Index.hpp"

namespace Contractor::Terms {

struct IndexPair {
	::Contractor::Terms::Index first;
	::Contractor::Terms::Index second;

	IndexPair() = default;
	IndexPair(const Index &first, const Index &second) : first(first), second(second) {}
	IndexPair(Index &&first, Index &&second) : first(first), second(second) {}
	IndexPair(const IndexPair &other) = default;
	IndexPair(IndexPair &&other)      = default;

	IndexPair &operator=(const IndexPair &other) = default;
	IndexPair &operator=(IndexPair &&other) = default;

	friend bool operator==(const IndexPair &lhs, const IndexPair &rhs) {
		return Index::isSame(lhs.first, rhs.first) && Index::isSame(lhs.second, rhs.second);
	}

	friend bool operator!=(const IndexPair &lhs, const IndexPair &rhs) { return !(lhs == rhs); }
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEXPAIR_HPP_
