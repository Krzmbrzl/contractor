#include "terms/BinaryTerm.hpp"
#include "terms/Tensor.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(BinaryTermTest, getter) {
	ct::Tensor parent("P");
	ct::Tensor left("L");
	ct::Tensor right("R");
	constexpr double prefactor = 1;

	ct::BinaryTerm term(parent, prefactor, left, right);

	ASSERT_EQ(term.size(), 2);
	ASSERT_EQ(term.getPrefactor(), prefactor);
	ASSERT_EQ(term.getParent(), parent);

	auto it = term.getTensors().begin();

	ASSERT_EQ(*it, left);
	++it;
	ASSERT_EQ(*it, right);
	++it;
	ASSERT_EQ(it, term.getTensors().end());
}
