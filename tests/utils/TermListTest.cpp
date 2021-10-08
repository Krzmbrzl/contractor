#include "utils/TermList.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

TEST(TermListTest, order) {
	ct::Tensor A("A");
	ct::Tensor B("B");
	ct::Tensor C("C");
	ct::Tensor D("D");
	ct::Tensor E("E");

	{
		// A depends on B and C, B depends on C.
		// Thus the correct order has to be C,B,A
		ct::GeneralTerm term1(A, 1.0, { B, C });
		ct::GeneralTerm term2(B, 1.0, { D, C });
		ct::GeneralTerm term3(C, 1.0, { E });

		cu::TermList list;
		list.add(term1, false);
		list.add(term2, false);
		list.add(term3, true);

		ASSERT_EQ(list[0], term3);
		ASSERT_EQ(list[1], term2);
		ASSERT_EQ(list[2], term1);
	}
	{
		ct::GeneralTerm term1(A, 1.0, { B, C });
		ct::GeneralTerm term2(B, 1.0, { D, C });
		ct::GeneralTerm term3(A, 2.0, { B });
		ct::GeneralTerm term4(C, 1.0, { E });

		cu::TermList list;
		list.add(term1, false);
		list.add(term2, false);
		list.add(term3, false);
		list.add(term4, true);

		// There is no strict ordering between term1 and term3 and thus their order
		// remains unspecified
		ASSERT_EQ(list[0], term4);
		ASSERT_EQ(list[1], term2);
		ASSERT_THAT(list[2], ::testing::AnyOf(::testing::Eq(term1), ::testing::Eq(term3)));
		ASSERT_THAT(list[3], ::testing::AnyOf(::testing::Eq(term1), ::testing::Eq(term3)));
		ASSERT_NE(list[2], list[3]);
	}
}

TEST(TermListTest, replace) {
	ct::Tensor A("A");
	ct::Tensor B("B");
	ct::Tensor C("C");
	ct::Tensor B_alt("B_alt");

	{
		ct::GeneralTerm term1(A, 2.0, { B });
		ct::GeneralTerm term2(A, 3.0, { B, C });
		ct::GeneralTerm term3(B, 4.0, { C });

		cu::TermList list;
		list.add(term1, false);
		list.add(term2, false);
		list.add(term3, true);

		ct::GeneralTerm term1_alt(A, 2.0, { B_alt });
		ct::GeneralTerm term2_alt(A, 3.0, { B_alt, C });

		list.replace(B, B_alt);

		ASSERT_EQ(list[0], term3);
		ASSERT_THAT(list[1], ::testing::AnyOf(::testing::Eq(term1_alt), ::testing::Eq(term2_alt)));
		ASSERT_THAT(list[2], ::testing::AnyOf(::testing::Eq(term1_alt), ::testing::Eq(term2_alt)));
		ASSERT_NE(list[1], list[2]);
	}
	{
		ct::Tensor B_symAlt("B", { idx("i"), idx("a") });
		ct::PermutationGroup symmetry(B_symAlt.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, 1));
		B_symAlt.setSymmetry(symmetry);

		ct::GeneralTerm term1(A, 2.0, { B });
		ct::GeneralTerm term2(A, 3.0, { B, C });
		ct::GeneralTerm term3(B, 4.0, { C });

		cu::TermList list;
		list.add(term1, false);
		list.add(term2, false);
		list.add(term3, true);

		ct::GeneralTerm term1_alt(A, 2.0, { B_symAlt });
		ct::GeneralTerm term2_alt(A, 3.0, { B_symAlt, C });
		ct::GeneralTerm term3_alt(B_symAlt, 4.0, { C });

		list.replace(B, B_symAlt, true);

		ASSERT_EQ(list[0], term3_alt);
		ASSERT_THAT(list[1], ::testing::AnyOf(::testing::Eq(term1_alt), ::testing::Eq(term2_alt)));
		ASSERT_THAT(list[2], ::testing::AnyOf(::testing::Eq(term1_alt), ::testing::Eq(term2_alt)));
		ASSERT_NE(list[1], list[2]);
	}
}
