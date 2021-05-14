#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(GeneralTermTest, getter) {
	ct::Tensor parent("H");
	constexpr ct::Term::factor_t prefactor = 1;
	ct::GeneralTerm term(parent, prefactor);

	ASSERT_EQ(term.size(), 0);
	ASSERT_EQ(term.getResult(), parent);
	ASSERT_EQ(term.getPrefactor(), prefactor);

	ct::Tensor a("A");
	ct::Tensor b("B");
	ct::GeneralTerm term2(parent, prefactor, { ct::Tensor(a), ct::Tensor(b) });

	ASSERT_EQ(term2.size(), 2);
	ASSERT_EQ(term2.getResult(), parent);
	ASSERT_EQ(term2.getPrefactor(), prefactor);

	auto it = term2.getTensors().begin();
	ASSERT_EQ(*it, a);
	++it;
	ASSERT_EQ((*it), b);
	++it;
	ASSERT_EQ(it, term2.getTensors().end());
}

TEST(GeneralTerm, add) {
	ct::Tensor parent("H");
	constexpr ct::Term::factor_t prefactor = 1;
	ct::GeneralTerm term(parent, prefactor);

	ct::Tensor additional("M");
	term.add(additional);

	ASSERT_EQ(term.size(), 1);
	ASSERT_EQ(*term.getTensors().begin(), additional);

	ct::Tensor another("A");
	ct::Tensor copy(another);
	term.add(std::move(another));

	ASSERT_EQ(term.size(), 2);
	auto it = term.getTensors().begin();
	++it;
	ASSERT_EQ(*it, copy);
}

TEST(GeneralTerm, remove) {
	ct::Tensor parent("H");
	constexpr ct::Term::factor_t prefactor = 1;
	ct::GeneralTerm term(parent, prefactor);

	ct::Tensor a("A");
	ct::Tensor b("B");
	ct::Tensor c("C");

	term.add(a);
	term.add(b);

	// Precondition
	ASSERT_EQ(term.size(), 2);

	ASSERT_FALSE(term.remove(c));
	ASSERT_EQ(term.size(), 2);

	ASSERT_TRUE(term.remove(b));
	ASSERT_EQ(term.size(), 1);

	ASSERT_FALSE(term.remove(b));
	ASSERT_EQ(term.size(), 1);

	ASSERT_TRUE(term.remove(a));
	ASSERT_EQ(term.size(), 0);
}
