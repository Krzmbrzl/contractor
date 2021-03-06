#include "terms/Term.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace Contractor::Terms {

Term::Term(const Tensor &result, Term::factor_t prefactor) : m_result(result), m_prefactor(prefactor) {
}

bool operator==(const Term &lhs, const Term &rhs) {
	return lhs.equals(rhs, Term::CompareOption::NONE);
}

bool operator!=(const Term &lhs, const Term &rhs) {
	return !(lhs == rhs);
}

bool operator<(const Term &lhs, const Term &rhs) {
	if (lhs.getResult() != rhs.getResult()) {
		return lhs.getResult() < rhs.getResult();
	}

	if (lhs.size() != rhs.size()) {
		return lhs.size() < rhs.size();
	}

	auto lhsIt = lhs.getTensors().begin();
	auto rhsIt = rhs.getTensors().begin();
	while (lhsIt != lhs.getTensors().end()) {
		if (*lhsIt != *rhsIt) {
			return *lhsIt < *rhsIt;
		}

		lhsIt++;
		rhsIt++;
	}

	return lhs.getPrefactor() < rhs.getPrefactor();
}

std::ostream &operator<<(std::ostream &stream, const Term &term) {
	stream << term.getResult() << " = " << term.getPrefactor() << " * ";
	for (const Tensor &currentTensor : term.getTensors()) {
		stream << currentTensor << " ";
	}
	return stream;
}

const Tensor &Term::getResult() const {
	return m_result;
}

void Term::setResult(const Tensor &result) {
	m_result = result;
}

Tensor &Term::accessResult() {
	return m_result;
}

Term::factor_t Term::getPrefactor() const {
	return m_prefactor;
}

void Term::setPrefactor(Term::factor_t prefactor) {
	m_prefactor = prefactor;
}

Iterable< const Tensor > Term::getTensors() const {
	// Get a function pointer to UnoptimizedTerm::get with which we can create the Iterable object that is to
	// be returned. Note that thanks to that get function not being overloaded, we don't have to perform any
	// overload resolution with evil looking casts :)
	typedef const Tensor &(Term::*const_getter_t)(std::size_t) const;

	std::function< const Tensor &(std::size_t) > getterFunction =
		std::bind(static_cast< const_getter_t >(&Term::get), this, std::placeholders::_1);

	return Iterable< const Tensor >(0, size(), getterFunction, this);
}

Iterable< Tensor > Term::accessTensors() {
	// Get a function pointer to UnoptimizedTerm::get with which we can create the Iterable object that is to
	// be returned. Note that thanks to that get function not being overloaded, we don't have to perform any
	// overload resolution with evil looking casts :)
	typedef Tensor &(Term::*getter_t)(std::size_t);

	std::function< Tensor &(std::size_t) > getterFunction =
		std::bind(static_cast< getter_t >(&Term::get), this, std::placeholders::_1);

	return Iterable< Tensor >(0, size(), getterFunction, this);
}

bool Term::equals(const Term &other, std::underlying_type_t< CompareOption::Options > options) const {
	if (size() != other.size()) {
		return false;
	}

	if (getPrefactor() != other.getPrefactor()) {
		return false;
	}

	if (getResult() != other.getResult()) {
		return false;
	}

	if ((options & CompareOption::REQUIRE_SAME_TYPE) && typeid(*this).hash_code() != typeid(other).hash_code()) {
		return false;
	}

	auto iterable1 = getTensors();
	auto iterable2 = other.getTensors();

	if (options & CompareOption::REQUIRE_SAME_ORDER) {
		return std::equal(iterable1.begin(), iterable1.end(), iterable2.begin());
	} else {
		return std::is_permutation(iterable1.begin(), iterable1.end(), iterable2.begin());
	}
}

void Term::deduceSymmetry() {
	PermutationGroup symmetry(getResult().getIndices());

	for (const Tensor &currentTensor : getTensors()) {
		for (const IndexSubstitution &currentSymmetry : currentTensor.getSymmetry().getGenerators()) {
			if (currentSymmetry.appliesTo(getResult())) {
				symmetry.addGenerator(currentSymmetry, false);
			}
		}
	}

	symmetry.regenerateGroup();

	accessResult().setSymmetry(std::move(symmetry));
}

Term::IndexSet Term::getUniqueIndices() const {
	Term::IndexSet indices;

	// Start with the result indices
	for (const Index &currentIndex : getResult().getIndices()) {
		indices.insert(currentIndex);
	}

	for (const Tensor &currentTensor : getTensors()) {
		for (const Index &currentIndex : currentTensor.getIndices()) {
			indices.insert(currentIndex);
		}
	}

	return indices;
}

Term::FormalScalingMap Term::getFormalScaling() const {
	Term::FormalScalingMap scaling;

	for (const Index &currentIndex : getUniqueIndices()) {
		scaling[currentIndex.getSpace()] += 1;
	}

	return scaling;
}

bool Term::isValid() const {
	try {
		assertIsValid();

		return true;
	} catch (const std::runtime_error &) {
		return false;
	}
}

void Term::assertIsValid() const {
	// Count how often each index occurs
	std::unordered_map< Index, unsigned int, Index::type_and_spin_insensitive_hasher, Index::index_has_same_name >
		indices;

	for (const Tensor &currentTensor : getTensors()) {
		for (const Index &currentIndex : currentTensor.getIndices()) {
			indices[currentIndex]++;
		}
	}

	Tensor::index_list_t resultIndices;

	for (const auto &currentPair : indices) {
		if (currentPair.second > 2) {
			// Indices can't occur more than twice
			throw std::runtime_error("Index occurs more than twice");
		}
		if (currentPair.second == 1) {
			// This is a result index
			resultIndices.push_back(currentPair.first);
		}
	}

	if (getResult().getIndices().size() != resultIndices.size()
		|| !std::is_permutation(resultIndices.begin(), resultIndices.end(), getResult().getIndices().begin(),
								Index::index_has_same_name{})) {
		// Inconsistent result indices
		throw std::runtime_error("Result indices are inconsistent");
	}
}

}; // namespace Contractor::Terms
