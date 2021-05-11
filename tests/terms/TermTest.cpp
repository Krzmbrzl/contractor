#include "terms/Term.hpp"
#include <terms/Tensor.hpp>

#include <algorithm>

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

struct DummyTerm : public ct::Term {
	static ct::Tensor PARENT;
	static constexpr Term::factor_t PREFACTOR = 1;
	static constexpr std::size_t SIZE         = 5;

	DummyTerm(std::size_t size = SIZE) : Term(PARENT, PREFACTOR) {
		for (std::size_t i = 0; i < size; i++) {
			m_tensors.push_back(ct::Tensor(std::to_string(i)));
		}
	}

	virtual std::size_t size() const override { return m_tensors.size(); }

	virtual const ct::Tensor &get(std::size_t index) const override { return m_tensors[index]; }

	std::vector< ct::Tensor > m_tensors;
};

struct DummyTerm2 : public DummyTerm {
	using DummyTerm::DummyTerm;
};

ct::Tensor DummyTerm::PARENT = ct::Tensor("P");

TEST(TermTest, getter) {
	DummyTerm term;

	ASSERT_EQ(term.size(), DummyTerm::SIZE);
	ASSERT_EQ(term.getParent(), DummyTerm::PARENT);
	ASSERT_EQ(term.getPrefactor(), DummyTerm::PREFACTOR);

	auto it = term.getTensors().begin();
	for (std::size_t i = 0; i < term.size(); i++) {
		ASSERT_EQ(*it, term.m_tensors[i]) << "Failed in iteration " << i;
		++it;
	}
	ASSERT_EQ(it, term.getTensors().end());
}

TEST(TermTest, equality) {
	DummyTerm term1(5);
	DummyTerm2 term2(5);
	DummyTerm term3(2);

	ASSERT_EQ(term1.equals(term2), true);
	ASSERT_EQ(term1.equals(term3), false);
	ASSERT_EQ(term2.equals(term3), false);

	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::IGNORE_ORDER), true);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::REQUIRE_SAME_TYPE), false);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::IGNORE_ORDER | ct::Term::CompareOption::REQUIRE_SAME_TYPE),
			  false);

	std::swap(term2.m_tensors[1], term2.m_tensors[2]);
	// Now that we have changed the order inside term2, when comparing term-by-term (order-aware) the equality
	// no longer holds, whereas a order-unaware compare still has to result in equality
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::IGNORE_ORDER), true);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::NONE), false);
}
