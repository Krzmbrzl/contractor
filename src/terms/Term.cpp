#include "terms/Term.hpp"

#include <algorithm>
#include <typeinfo>

namespace Contractor::Terms {

Term::Term(const Tensor &result, Term::factor_t prefactor) : m_result(result), m_prefactor(prefactor) {
}

bool operator==(const Term &lhs, const Term &rhs) {
	return lhs.equals(rhs, Term::CompareOption::NONE);
}

bool operator!=(const Term &lhs, const Term &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &stream, const Term &term) {
	stream << "Term (" << typeid(term).name() << "):\n";
	stream << "- Result: " << term.getResult() << "\n";
	stream << "- Factor: " << term.getPrefactor() << "\n";
	for (const Tensor &currentTensor : term.getTensors()) {
		stream << "-> " << currentTensor << "\n";
	}
	return stream;
}

const Tensor &Term::getResult() const {
	return m_result;
}

void Term::setResult(const Tensor &result) {
	m_result = result;
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
	std::function< const Tensor &(std::size_t) > getterFunction = std::bind(&Term::get, this, std::placeholders::_1);

	return Iterable< const Tensor >(0, size(), getterFunction, this);
}

bool Term::equals(const Term &other, std::underlying_type_t< CompareOption::Options > options) const {
	if (size() != other.size()) {
		return false;
	}

	if (getPrefactor() != other.getPrefactor()) {
		return false;
	}

	if ((options & CompareOption::REQUIRE_SAME_TYPE) && typeid(*this).hash_code() != typeid(other).hash_code()) {
		return false;
	}

	if (options & CompareOption::IGNORE_ORDER) {
		return commutativeEquals(other);
	} else {
		return nonCommutativeEquals(other);
	}
}

bool Term::commutativeEquals(const Term &other) const {
	auto otherTerms = other.getTensors();

	for (const Tensor &currentTerm : getTensors()) {
		if (std::find(otherTerms.begin(), otherTerms.end(), currentTerm) == otherTerms.end()) {
			return false;
		}
	}

	return true;
}

bool Term::nonCommutativeEquals(const Term &other) const {
	if (size() != other.size()) {
		return false;
	}

	auto iterable1 = getTensors();
	auto iterable2 = other.getTensors();

	return std::equal(iterable1.begin(), iterable1.end(), iterable2.begin());
}

}; // namespace Contractor::Terms
