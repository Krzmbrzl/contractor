#include "terms/CompositeTerm.hpp"

#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace ct = Contractor::Terms;

static ct::GeneralCompositeTerm composite1({
	ct::GeneralTerm(ct::Tensor("A1"), 2, { ct::Tensor("B") }),
	ct::GeneralTerm(ct::Tensor("A1"), 3, { ct::Tensor("C") }),
});
// Same Term again but with a different result name and different Term order
static ct::GeneralCompositeTerm composite2({
	ct::GeneralTerm(ct::Tensor("A2"), 3, { ct::Tensor("C") }),
	ct::GeneralTerm(ct::Tensor("A2"), 2, { ct::Tensor("B") }),
});
// Same Term but the prefactors are halfed
static ct::GeneralCompositeTerm composite3({
	ct::GeneralTerm(ct::Tensor("A3"), 2 / 2.0, { ct::Tensor("B") }),
	ct::GeneralTerm(ct::Tensor("A3"), 3 / 2.0, { ct::Tensor("C") }),
});
// Reversed prefactors
static ct::GeneralCompositeTerm composite4({
	ct::GeneralTerm(ct::Tensor("A4"), 2, { ct::Tensor("C") }),
	ct::GeneralTerm(ct::Tensor("A4"), 3, { ct::Tensor("B") }),
});
// Different Term
static ct::GeneralCompositeTerm composite5({
	ct::GeneralTerm(ct::Tensor("A5"), 2, { ct::Tensor("C") }),
	ct::GeneralTerm(ct::Tensor("A5"), 3, { ct::Tensor("D") }),
});

TEST(CompositeTermTest, isRelatedTo) {
	std::vector< ct::GeneralCompositeTerm > relatedTerms   = { composite1, composite2, composite3 };
	std::vector< ct::GeneralCompositeTerm > unrelatedTerms = { composite4, composite5 };

	for (const ct::GeneralCompositeTerm &left : relatedTerms) {
		for (const ct::GeneralCompositeTerm &right : relatedTerms) {
			std::cout << "Checking whether" << std::endl
					  << left << std::endl
					  << "is related to" << std::endl
					  << right << std::endl;
			ASSERT_TRUE(left.isRelatedTo(right));
		}
	}

	for (const ct::GeneralCompositeTerm &first : relatedTerms) {
		for (const ct::GeneralCompositeTerm &second : unrelatedTerms) {
			std::cout << "Checking whether" << std::endl
					  << first << std::endl
					  << "is NOT related to" << std::endl
					  << second << std::endl;
			ASSERT_FALSE(first.isRelatedTo(second));
			ASSERT_FALSE(second.isRelatedTo(first));
		}
	}
}

TEST(CompositeTermTest, getRelation) {
	{
		ct::TensorSubstitution relation = composite1.getRelation(composite2);

		ct::TensorSubstitution expectedRelation(composite1.getResult(), composite2.getResult(), 1);

		ASSERT_EQ(relation, expectedRelation);
	}
	{
		ct::TensorSubstitution relation = composite2.getRelation(composite1);

		ct::TensorSubstitution expectedRelation(composite2.getResult(), composite1.getResult(), 1);

		ASSERT_EQ(relation, expectedRelation);
	}
	{
		ct::TensorSubstitution relation = composite1.getRelation(composite3);

		ct::TensorSubstitution expectedRelation(composite1.getResult(), composite3.getResult(), 2);

		ASSERT_EQ(relation, expectedRelation);
	}
	{
		ct::TensorSubstitution relation = composite3.getRelation(composite1);

		ct::TensorSubstitution expectedRelation(composite3.getResult(), composite1.getResult(), 0.5);

		ASSERT_EQ(relation, expectedRelation);
	}
	{
		ct::TensorSubstitution relation = composite2.getRelation(composite3);

		ct::TensorSubstitution expectedRelation(composite2.getResult(), composite3.getResult(), 2);

		ASSERT_EQ(relation, expectedRelation);
	}
	{
		ct::TensorSubstitution relation = composite3.getRelation(composite2);

		ct::TensorSubstitution expectedRelation(composite3.getResult(), composite2.getResult(), 0.5);

		ASSERT_EQ(relation, expectedRelation);
	}
}
