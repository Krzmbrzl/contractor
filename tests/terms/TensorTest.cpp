#include "terms/Tensor.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/PermutationGroup.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include "IndexHelper.hpp"

#include <string_view>
#include <unordered_map>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

using ExponentsMap = decltype(ct::ContractionResult::spaceExponents);

TEST(TensorTest, getter) {
	ct::Tensor empty1("H");
	ct::Tensor empty2("T2");

	ASSERT_EQ(empty1.getName(), "H");
	ASSERT_EQ(empty2.getName(), "T2");
	ASSERT_EQ(empty1.getIndices().size(), 0);
	ASSERT_EQ(empty2.getIndices().size(), 0);

	ct::Tensor::index_list_t creators = { idx("j+"), idx("k+"), idx("a+"), idx("c+") };

	ct::Tensor creatorsOnly("H", creators);

	ASSERT_EQ(creatorsOnly.getName(), "H");
	ASSERT_EQ(creatorsOnly.getIndices(), creators);

	ct::Tensor::index_list_t annihilators = { idx("a"), idx("j"), idx("n"), idx("b") };

	ct::Tensor annihilatorsOnly("H", annihilators);

	ASSERT_EQ(annihilatorsOnly.getName(), "H");
	ASSERT_EQ(annihilatorsOnly.getIndices(), annihilators);


	ct::IndexSubstitution permuation = ct::IndexSubstitution::createPermutation({ { creators[0], creators[1] } });
	ct::PermutationGroup symmetry(creatorsOnly.getIndices());
	symmetry.addGenerator(permuation);
	creatorsOnly.setSymmetry(symmetry);
	ASSERT_EQ(creatorsOnly.getSymmetry(), symmetry);
}

TEST(TensorTest, equality) {
	{
		ct::Tensor element1("H");
		ct::Tensor element2("T2");
		ct::Tensor element3("H");

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", { idx("i+") });
		ct::Tensor element2("H");
		ct::Tensor element3("H", { idx("i+") });

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", { idx("i+") });
		ct::Tensor element2("H", { idx("j+") });
		ct::Tensor element3("H", { idx("i+") });

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", { idx("i+") });
		ct::Tensor element2("H", { idx("a+") });
		ct::Tensor element3("H", { idx("i+") });

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::IndexSubstitution permuation = ct::IndexSubstitution::createPermutation({ { idx("i+"), idx("j+") } });

		ct::Tensor element1("H", { idx("i+"), idx("j+") });
		ct::Tensor element2("H", { idx("a+"), idx("j+") });
		ct::Tensor element3("H", { idx("i+"), idx("j+") });

		ct::PermutationGroup symmetry(element1.getIndices());
		symmetry.addGenerator(permuation);
		element1.setSymmetry(symmetry);

		symmetry.setRootSequence(element3.getIndices());
		element3.setSymmetry(symmetry);

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}
}

ct::Tensor createTensor(const std::string_view name, std::size_t indexCount) {
	ct::Tensor::index_list_t indices;

	for (std::size_t i = 0; i < indexCount; i++) {
		indices.push_back(ct::Index(ct::IndexSpace(i % 5), i, ct::Index::Type::None));
	}

	return ct::Tensor(name, std::move(indices));
}

TEST(TensorTest, helperFunction) {
	ct::Tensor element = createTensor("ABC", 3);

	ASSERT_EQ(element.getName(), "ABC");
	ASSERT_EQ(element.getIndices().size(), 3);
}

TEST(TensorTest, copy) {
	ct::Tensor element = createTensor("H", 3);
	ct::Tensor copy    = ct::Tensor(element);

	ASSERT_EQ(element, copy);
}

TEST(TensorTest, move) {
	ct::Tensor element = createTensor("H", 4);
	ct::Tensor copy    = ct::Tensor(element);

	ct::Tensor newTensor = std::move(element);

	// After the move the new element should be what element was before the move whilst element
	// is now basically an empty hull
	ASSERT_EQ(newTensor, copy);
	ASSERT_EQ(element.getName(), "");
	ASSERT_EQ(element.getIndices().size(), 0);
}

TEST(TensorTest, refersToSameElement) {
	{
		ct::Tensor first("g");
		ct::Tensor second("g");

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g");
		ct::Tensor second("p");

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_FALSE(first.refersToSameElement(second));
		ASSERT_FALSE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+") });
		ct::Tensor second("g", { idx("j+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+") });
		ct::Tensor second("g", { idx("a+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_FALSE(first.refersToSameElement(second));
		ASSERT_FALSE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("k+") });
		ct::Tensor second("g", { idx("l+"), idx("m+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("j+") });
		ct::Tensor second("g", { idx("k+"), idx("l") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_FALSE(first.refersToSameElement(second));
		ASSERT_FALSE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("i+") });
		ct::Tensor second("g", { idx("k+"), idx("l+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_FALSE(first.refersToSameElement(second));
		ASSERT_FALSE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("i+") });
		ct::Tensor second("g", { idx("k+"), idx("k+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("i+"), idx("j+") });
		ct::Tensor second("g", { idx("k+"), idx("k+"), idx("k+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_FALSE(first.refersToSameElement(second));
		ASSERT_FALSE(second.refersToSameElement(first));
	}
	{
		ct::Tensor first("g", { idx("i+"), idx("j+"), idx("i+") });
		ct::Tensor second("g", { idx("k+"), idx("l+"), idx("k+") });

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
	{
		// This function is also supposed to take symmetry into account
		ct::Tensor first("T", { idx("a+"), idx("i-") });
		first.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("i") } }));

		ct::Tensor second("T", { idx("i+"), idx("a-") });
		second.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }));

		ASSERT_TRUE(first.refersToSameElement(first));
		ASSERT_TRUE(second.refersToSameElement(second));
		ASSERT_TRUE(first.refersToSameElement(second));
		ASSERT_TRUE(second.refersToSameElement(first));
	}
}

TEST(TensorTest, transferSymmetry) {
	{
		// Nothing to transfer -> no change
		const ct::Tensor symmetrySource("H");
		const ct::Tensor original("H");
		ct::Tensor transformed(original);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, original);
	}
	{
		ct::Index si1                          = idx("i+");
		ct::Index si2                          = idx("j+");
		ct::Index si3                          = idx("j");
		ct::Index si4                          = idx("j");
		ct::Tensor::index_list_t sourceIndices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor symmetrySource("H", sourceIndices);
		ct::PermutationGroup sourceSymmetry(symmetrySource.getIndices());
		sourceSymmetry.addGenerator(ct::IndexSubstitution({ si1, si2 }));
		symmetrySource.setSymmetry(sourceSymmetry);

		ct::Index i1                     = idx("k+");
		ct::Index i2                     = idx("l+");
		ct::Index i3                     = idx("k");
		ct::Index i4                     = idx("l");
		ct::Tensor::index_list_t indices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };

		ct::Tensor expected("H", indices);
		ct::Tensor transformed = expected;

		ct::PermutationGroup expectedSymmetry(expected.getIndices());
		expectedSymmetry.addGenerator(ct::IndexSubstitution({ si1, si2 }));
		expected.setSymmetry(expectedSymmetry);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		ct::Index si1                          = idx("i+");
		ct::Index si2                          = idx("j+");
		ct::Index si3                          = idx("j");
		ct::Index si4                          = idx("j");
		ct::Tensor::index_list_t sourceIndices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor symmetrySource("H", sourceIndices);

		ct::PermutationGroup sourceSymmetry(symmetrySource.getIndices());
		sourceSymmetry.addGenerator(ct::IndexSubstitution({ si1, si2 }));
		symmetrySource.setSymmetry(sourceSymmetry);

		ct::Index i1                     = idx("l+");
		ct::Index i2                     = idx("k+");
		ct::Index i3                     = idx("j");
		ct::Index i4                     = idx("m");
		ct::Tensor::index_list_t indices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor expected("H", indices);
		ct::Tensor transformed = expected;

		ct::PermutationGroup expectedSymmetry(expected.getIndices());
		expectedSymmetry.addGenerator(ct::IndexSubstitution({ si1, si2 }));
		expected.setSymmetry(expectedSymmetry);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		// Duplicate indices
		ct::Index si1                          = idx("i+");
		ct::Index si2                          = idx("i+");
		ct::Index si3                          = idx("j");
		ct::Index si4                          = idx("j");
		ct::Tensor::index_list_t sourceIndices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor symmetrySource("H", sourceIndices);

		ct::PermutationGroup sourceSymmetry(symmetrySource.getIndices());
		sourceSymmetry.addGenerator(ct::IndexSubstitution({ si1, si3 }));
		symmetrySource.setSymmetry(sourceSymmetry);

		ct::Index i1                     = idx("p+");
		ct::Index i2                     = idx("p+");
		ct::Index i3                     = idx("j");
		ct::Index i4                     = idx("l");
		ct::Tensor::index_list_t indices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor expected("H", indices);
		ct::Tensor transformed = expected;

		ct::PermutationGroup expectedSymmetry(expected.getIndices());
		expectedSymmetry.addGenerator(ct::IndexSubstitution({ si1, si3 }));
		expected.setSymmetry(expectedSymmetry);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		// Destination tensor already has some symmetry (which will be overwritten)
		ct::Index si1                          = idx("i+");
		ct::Index si2                          = idx("j+");
		ct::Index si3                          = idx("j");
		ct::Index si4                          = idx("j");
		ct::Tensor::index_list_t sourceIndices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor symmetrySource("H", sourceIndices);

		ct::PermutationGroup sourceSymmetry(symmetrySource.getIndices());
		sourceSymmetry.addGenerator(ct::IndexSubstitution({ si1, si3 }));
		symmetrySource.setSymmetry(sourceSymmetry);

		ct::Index i1                     = idx("l+");
		ct::Index i2                     = idx("k+");
		ct::Index i3                     = idx("j");
		ct::Index i4                     = idx("m");
		ct::Tensor::index_list_t indices = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor expected("H", indices);
		ct::Tensor transformed = expected;

		ct::PermutationGroup transformedSymmtry(transformed.getIndices());
		transformedSymmtry.addGenerator(ct::IndexSubstitution({ i3, i4 }));
		transformed.setSymmetry(transformedSymmtry);

		ct::PermutationGroup expectedSymmetry(expected.getIndices());
		expectedSymmetry.addGenerator(ct::IndexSubstitution({ si1, si3 }));
		expected.setSymmetry(expectedSymmetry);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		ct::Tensor source("H", { idx("i+"), idx("a+"), idx("b"), idx("j") });
		ct::Tensor target("H", { idx("k+"), idx("b+"), idx("c"), idx("j") });

		ct::PermutationGroup sourceSymmetry(source.getIndices());
		sourceSymmetry.addGenerator(ct::IndexSubstitution::createCyclicPermutation({ { idx("a"), idx("i") } }, -1),
									false);
		sourceSymmetry.addGenerator(ct::IndexSubstitution::createCyclicPermutation({ { idx("b"), idx("j") } }, -1));
		source.setSymmetry(sourceSymmetry);

		ct::Tensor expectedResult = target;

		ct::PermutationGroup expectedSymmetry(expectedResult.getIndices());
		expectedSymmetry.addGenerator(ct::IndexSubstitution::createCyclicPermutation({ { idx("k"), idx("b") } }, -1),
									  false);
		expectedSymmetry.addGenerator(ct::IndexSubstitution::createCyclicPermutation({ { idx("c"), idx("j") } }, -1));
		expectedResult.setSymmetry(expectedSymmetry);

		ct::Tensor::transferSymmetry(source, target);

		ASSERT_EQ(target, expectedResult);
	}
}

TEST(TensorTest, getIndexMapping) {
	ct::Index i(ct::IndexSpace(0), 0, ct::Index::Type::Annihilator);
	ct::Index j(ct::IndexSpace(0), 1, ct::Index::Type::Annihilator);
	ct::Index a(ct::IndexSpace(1), 0, ct::Index::Type::Creator);
	ct::Index b(ct::IndexSpace(1), 1, ct::Index::Type::Creator);

	ct::Index k(i);
	k.setID(5);
	ct::Index l(j);
	l.setID(6);
	ct::Index c(a);
	c.setID(5);
	ct::Index d(b);
	d.setID(6);

	ASSERT_NE(i, k);
	ASSERT_NE(j, l);
	ASSERT_NE(a, c);
	ASSERT_NE(b, d);

	ct::Tensor one("H", { ct::Index(i), ct::Index(j), ct::Index(a), ct::Index(a), ct::Index(b) });
	ct::Tensor two("H", { ct::Index(k), ct::Index(l), ct::Index(c), ct::Index(c), ct::Index(d) });

	ct::IndexSubstitution expectedMapping(
		{ ct::IndexSubstitution::index_pair_t(i, k), ct::IndexSubstitution::index_pair_t(j, l),
		  ct::IndexSubstitution::index_pair_t(a, c), ct::IndexSubstitution::index_pair_t(b, d) },
		1);

	ASSERT_TRUE(one.refersToSameElement(two, false));

	ASSERT_EQ(one.getIndexMapping(two), expectedMapping);
}

TEST(TensorTest, contract) {
	ct::ContractionResult::cost_t occupiedSize = resolver.getMeta(idx("i").getSpace()).getSize();
	ct::ContractionResult::cost_t virtualSize  = resolver.getMeta(idx("a").getSpace()).getSize();
	{
		// Contraction over a single common index
		ct::Tensor t1("T1", { idx("i+") });
		ct::Tensor t2("T2", { idx("i") });

		ct::Tensor expectedResult("T1_T2", {});
		ct::ContractionResult::cost_t expectedCost = occupiedSize;
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 1;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
	{
		// No contraction possible
		ct::Tensor t1("T1", { idx("i+") });
		ct::Tensor t2("T2", { idx("j") });

		ct::Tensor expectedResult("T1_T2", { idx("i+"), idx("j") });
		ct::ContractionResult::cost_t expectedCost = pow(occupiedSize, 2);
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 2;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
	{
		// Contraction over a single common index with other indices present as well
		ct::Tensor t1("T1", { idx("a"), idx("i+") });
		ct::Tensor t2("T2", { idx("i"), idx("b+") });

		ct::Tensor expectedResult("T1_T2", { idx("a"), idx("b+") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize * pow(virtualSize, 2);
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 1;
		exponents[idx("a").getSpace()] = 2;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
	{
		// Contraction over two common indices
		ct::Tensor t1("T1", { idx("a"), idx("i+") });
		ct::Tensor t2("T2", { idx("i"), idx("b"), idx("a+") });

		ct::Tensor expectedResult("T1_T2", { idx("b") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize * pow(virtualSize, 2);
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 1;
		exponents[idx("a").getSpace()] = 2;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
	{
		// "Contraction" with scalar
		ct::Tensor t1("T1", { idx("i") });
		ct::Tensor t2("T2", {});

		ct::Tensor expectedResult("T1_T2", { idx("i") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize;
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 1;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
	{
		// "Contraction" with scalar (reversed)
		ct::Tensor t1("T1", { idx("i") });
		ct::Tensor t2("T2", {});

		ct::Tensor expectedResult("T1_T2", { idx("i") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize;
		ExponentsMap exponents;
		exponents[idx("i").getSpace()] = 1;

		ct::ContractionResult result = t2.contract(t1, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
		ASSERT_EQ(result.spaceExponents, exponents);
	}
}

TEST(TensorTest, contractWithSymmetry) {
	{
		// A_B[ij,ab] = A[i,a,q] B[j,b,q]
		ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, 1);
		ct::IndexSubstitution perm2 = ct::IndexSubstitution::createPermutation({ { idx("j"), idx("b") } }, 1);

		ct::Tensor t1("A", { idx("i+"), idx("a"), idx("q!") });
		ct::Tensor t2("B", { idx("j+"), idx("b"), idx("q!") });

		ct::PermutationGroup sym1(t1.getIndices());
		sym1.addGenerator(perm1);
		t1.setSymmetry(sym1);

		ct::PermutationGroup sym2(t2.getIndices());
		sym2.addGenerator(perm2);
		t2.setSymmetry(sym2);

		ct::Tensor expectedResult("A_B", { idx("i+"), idx("j+"), idx("a"), idx("b") });

		ct::PermutationGroup expectedSymmetry(expectedResult.getIndices());
		expectedSymmetry.addGenerator(perm1, false);
		expectedSymmetry.addGenerator(perm2, false);
		expectedSymmetry.regenerateGroup();
		expectedResult.setSymmetry(expectedSymmetry);

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
	}
	{
		// A_B[j,b] = A[ij,ab] B[a,i]
		ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, 1);
		ct::IndexSubstitution perm2 = ct::IndexSubstitution::createPermutation({ { idx("j"), idx("b") } }, 1);
		ct::IndexSubstitution perm3 = ct::IndexSubstitution::createPermutation({ { idx("a"), idx("i") } }, 1);

		ct::Tensor t1("A", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::Tensor t2("B", { idx("a+"), idx("i") });

		ct::PermutationGroup sym1(t1.getIndices());
		sym1.addGenerator(perm1);
		sym1.addGenerator(perm2);
		t1.setSymmetry(sym1);

		ct::PermutationGroup sym2(t2.getIndices());
		sym2.addGenerator(perm3);
		t2.setSymmetry(sym2);

		ct::Tensor expectedResult("A_B", { idx("j+"), idx("b") });

		ct::PermutationGroup expectedSymmetry(expectedResult.getIndices());
		expectedSymmetry.addGenerator(perm2);
		expectedResult.setSymmetry(expectedSymmetry);

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
	}
	{
		// A_B[j,b] = A[ij,ab] B[a,i]
		ct::IndexSubstitution perm = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("b") } }, -1);

		ct::Tensor t1("A", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::Tensor t2("B", { idx("a+"), idx("i") });

		ct::PermutationGroup sym(t1.getIndices());
		sym.addGenerator(perm);
		t1.setSymmetry(sym);

		ct::Tensor expectedResult("A_B", { idx("j+"), idx("b") }, {});

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.resultTensor, expectedResult);
	}
}

TEST(TensorTest, antisymmetric) {
	{
		// There are no exchanges possible and thus 100% of all exchanges are allowed
		ct::Tensor T("T");

		ASSERT_TRUE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i"), idx("j") });

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_FALSE(T.isPartiallyAntisymmetrized());
	}
	{
		// There are no exchanges possible and thus 100% of all exchanges are allowed
		ct::Tensor T("T", { idx("i+"), idx("j") });

		ASSERT_TRUE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i"), idx("j+") });

		ASSERT_TRUE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+") });

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_FALSE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_FALSE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::PermutationGroup symmetry(T.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		T.setSymmetry(symmetry);

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::PermutationGroup symmetry(T.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		T.setSymmetry(symmetry);

		ASSERT_TRUE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::PermutationGroup symmetry(T.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, 1));
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		T.setSymmetry(symmetry);

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::PermutationGroup symmetry(T.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		symmetry.addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") }, { idx("a"), idx("b") } }, 1));
		T.setSymmetry(symmetry);

		ASSERT_TRUE(T.isAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());
	}
	{
		ct::Tensor T("T", { idx("i+"), idx("j+"), idx("a"), idx("b") });
		ct::PermutationGroup symmetry(T.getIndices());
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, -1));
		symmetry.addGenerator(ct::IndexSubstitution::createPermutation({ { idx("j"), idx("b") } }, -1));
		T.setSymmetry(symmetry);

		ASSERT_FALSE(T.isAntisymmetrized());
		ASSERT_FALSE(T.isPartiallyAntisymmetrized());
	}
}

TEST(TensorTest, canonicalizeIndices) {
	{
		ct::Tensor tensor("T", { idx("b+"), idx("a+") });
		tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ASSERT_FALSE(tensor.hasCanonicalIndexSequence());
		ASSERT_EQ(tensor.canonicalizeIndices(), -1);
		ASSERT_TRUE(tensor.hasCanonicalIndexSequence());

		ASSERT_THAT(tensor.getIndices(), ::testing::ElementsAre(idx("a+"), idx("b+")));
	}
}
