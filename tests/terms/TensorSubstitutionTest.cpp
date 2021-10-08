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

TEST(TensorSubstitutionTest, realWorldExamples) {
	{
		// Apply DF_DF[i⁺j⁺a⁻b⁻](////) = DF_DF[i⁺j⁺a⁻b⁻](/\/\)
		// to DF_DF_T2[a⁺i⁺j⁻b⁻](////) += - DF_DF[k⁺i⁺b⁻c⁻](////) T2[a⁺c⁺k⁻j⁻](....)
		// and expect DF_DF_T2[a⁺i⁺j⁻b⁻](////) += - DF_DF[k⁺i⁺b⁻c⁻](/\/\) T2[a⁺c⁺k⁻j⁻](....)
		ct::Tensor DF_DF_aaaa("DF_DF", { idx("i+/"), idx("j+/"), idx("a-/"), idx("b-/") });
		ct::Tensor DF_DF_abab("DF_DF", { idx("i+/"), idx("j+\\"), idx("a-/"), idx("b-\\") });

		ct::TensorSubstitution substitution(DF_DF_aaaa, DF_DF_abab);

		ct::GeneralTerm term(ct::Tensor("DF_DF_T2", { idx("a+/"), idx("i+/"), idx("j-/"), idx("b-/") }), 1,
							 { ct::Tensor("DF_DF", { idx("k+/"), idx("i+/"), idx("b-/"), idx("c-/") }),
							   ct::Tensor("T2", { idx("a+|"), idx("c+|"), idx("k-|"), idx("j-|") }) });

		ct::GeneralTerm expectedTerm(ct::Tensor("DF_DF_T2", { idx("a+/"), idx("i+/"), idx("j-/"), idx("b-/") }), 1,
									 { ct::Tensor("DF_DF", { idx("k+/"), idx("i+\\"), idx("b-/"), idx("c-\\") }),
									   ct::Tensor("T2", { idx("a+|"), idx("c+|"), idx("k-|"), idx("j-|") }) });

		bool applied = substitution.apply(term);

		ASSERT_TRUE(applied);
		ASSERT_EQ(term, expectedTerm);
	}
}
