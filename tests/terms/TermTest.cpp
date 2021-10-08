#include "terms/Term.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>

#include <gtest/gtest.h>

#include "IndexHelper.hpp"

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
	virtual ct::Tensor &get(std::size_t index) override { return m_tensors[index]; }

	virtual void sort() override {}

	std::vector< ct::Tensor > m_tensors;
};

struct DummyTerm2 : public DummyTerm {
	using DummyTerm::DummyTerm;
};

ct::Tensor DummyTerm::PARENT = ct::Tensor("P");

TEST(TermTest, getter) {
	DummyTerm term;

	ASSERT_EQ(term.size(), DummyTerm::SIZE);
	ASSERT_EQ(term.getResult(), DummyTerm::PARENT);
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

	ASSERT_EQ(term1.equals(term2), true);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::REQUIRE_SAME_TYPE), false);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::REQUIRE_SAME_TYPE), false);

	std::swap(term2.m_tensors[1], term2.m_tensors[2]);
	// Now that we have changed the order inside term2, when comparing term-by-term (order-aware) the equality
	// no longer holds, whereas a order-unaware compare still has to result in equality
	ASSERT_EQ(term1.equals(term2), true);
	ASSERT_EQ(term1.equals(term2, ct::Term::CompareOption::REQUIRE_SAME_ORDER), false);
}

TEST(TermTest, deduceSymmetry) {
	{
		ct::Tensor result("R", { idx("i+"), idx("a") });
		ct::Tensor A("A", { idx("i+"), idx("j+") });

		ct::GeneralTerm term(result, 1.0, { A });

		term.deduceSymmetry();

		// No symmetry involved so nothing should change
		ASSERT_EQ(term.getResult(), result);
	}
	{
		ct::Tensor R("R", { idx("i+"), idx("a") });
		ct::IndexSubstitution permutation = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, 1);
		ct::Tensor result("R", { idx("i+"), idx("a") });
		ct::Tensor A("A", { idx("i+"), idx("a") });

		ct::PermutationGroup symmetry(result.getIndices());
		symmetry.addGenerator(permutation);
		result.setSymmetry(symmetry);

		symmetry.setRootSequence(A.getIndices());
		A.setSymmetry(symmetry);

		ct::GeneralTerm term(R, 1.0, { A });

		term.deduceSymmetry();

		ASSERT_EQ(term.getResult(), result);
	}
	{
		ct::Tensor R("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });

		ct::IndexSubstitution permutation = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1);

		ct::Tensor result("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });

		ct::PermutationGroup symmetry(result.getIndices());
		symmetry.addGenerator(permutation);
		result.setSymmetry(symmetry);

		ct::Tensor A("A", { idx("i+"), idx("j+"), idx("a"), idx("c") });

		symmetry.setRootSequence(A.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("c") } }, -1));
		A.setSymmetry(symmetry);

		ct::Tensor B("B", { idx("c+"), idx("b") }, {});

		ct::GeneralTerm term(R, 1.0, { A });

		term.deduceSymmetry();

		ASSERT_EQ(term.getResult(), result);
	}
}
