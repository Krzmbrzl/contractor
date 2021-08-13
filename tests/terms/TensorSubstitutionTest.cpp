#include "terms/TensorSubstitution.hpp"
#include "terms/GeneralTerm.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace ct = Contractor::Terms;

TEST(TensorSubstitutionTest, apply) {
	const ct::Tensor result("R", { idx("a+"), idx("i-") });
	const ct::Tensor A("A", { idx("a+"), idx("j-") });
	const ct::Tensor B("B", { idx("j+"), idx("i-") });
	const ct::GeneralTerm originalTerm(result, 2, { A, B });

	{
		// Substitution that doesn't apply
		ct::GeneralTerm term = originalTerm;
		ct::Tensor dummy("Dummy");

		ct::TensorSubstitution substitution(dummy, A);

		bool applied = substitution.apply(term);

		ASSERT_FALSE(applied);
		ASSERT_EQ(term, originalTerm);
	}
	{
		// Substitute A
		ct::GeneralTerm term = originalTerm;

		ct::TensorSubstitution substitution(A, B, -1);

		ct::GeneralTerm expectedTerm(result, -2, { B, B });

		bool applied = substitution.apply(term);

		ASSERT_TRUE(applied);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// By default we do substitute the result of a Term
		ct::GeneralTerm term = originalTerm;

		ct::TensorSubstitution substitution(result, A, 0.5);

		ct::GeneralTerm expectedTerm(A, 1, { A, B });

		bool applied = substitution.apply(term);

		ASSERT_TRUE(applied);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Explicitly exclude the result from being replaced
		ct::GeneralTerm term = originalTerm;

		ct::TensorSubstitution substitution(result, A, 0.5);

		bool applied = substitution.apply(term, false);

		ASSERT_FALSE(applied);
		ASSERT_EQ(term, originalTerm);
	}
	{
		// Substitute a Tensor that uses different index names
		ct::GeneralTerm term = originalTerm;

		const ct::Tensor A_alt("A", { idx("b+"), idx("k-") });
		const ct::Tensor A_new("A_new", { idx("b+"), idx("k-") });
		const ct::Tensor A_new_remapped("A_new", { idx("a+"), idx("j-") });

		ct::TensorSubstitution substitution(A_alt, A_new, -1);

		// We expect an index remapping to happen before the actual substitution is performed
		ct::GeneralTerm expectedTerm(result, -2, { A_new_remapped, B });

		bool applied = substitution.apply(term);

		ASSERT_TRUE(applied);
		ASSERT_EQ(term, expectedTerm);
	}
}
