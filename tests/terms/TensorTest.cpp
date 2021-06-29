#include "terms/Tensor.hpp"
#include "terms/IndexSubstitution.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include "IndexHelper.hpp"

#include <string_view>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;


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


	ct::IndexSubstitution p(ct::IndexSubstitution::index_pair_t(creators[0], creators[1]));
	ct::Tensor::symmetry_list_t permutations = { ct::IndexSubstitution(p) };
	creatorsOnly.setIndexSymmetries(permutations);
	ASSERT_EQ(creatorsOnly.getIndexSymmetries(), permutations);
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
		ct::IndexSubstitution p(ct::IndexSubstitution::index_pair_t(idx("i+"), idx("j+")));
		ct::Tensor::symmetry_list_t permutations = { ct::IndexSubstitution(p) };

		ct::Tensor element1("H", { idx("i+"), idx("j+") }, permutations);
		ct::Tensor element2("H", { idx("a+"), idx("j+") });
		ct::Tensor element3("H", { idx("i+"), idx("j+") }, permutations);

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
		ct::Index si1 = idx("i+");
		ct::Index si2 = idx("j+");
		ct::Index si3 = idx("j");
		ct::Index si4 = idx("j");
		ct::IndexSubstitution sp1(ct::IndexSubstitution::index_pair_t(si1, si2));
		ct::Tensor::index_list_t sourceIndices     = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t sourceSymmetry = { ct::IndexSubstitution(sp1) };
		const ct::Tensor symmetrySource("H", sourceIndices, sourceSymmetry);

		ct::Index i1 = idx("k+");
		ct::Index i2 = idx("l+");
		ct::Index i3 = idx("k");
		ct::Index i4 = idx("l");
		ct::IndexSubstitution p1(ct::IndexSubstitution::index_pair_t(si1, si2));
		ct::Tensor::index_list_t indices       = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t symmetries = { ct::IndexSubstitution(p1) };
		const ct::Tensor expected("H", indices, symmetries);
		ct::Tensor transformed("H", indices);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		ct::Index si1 = idx("i+");
		ct::Index si2 = idx("j+");
		ct::Index si3 = idx("j");
		ct::Index si4 = idx("j");
		ct::IndexSubstitution sp1(ct::IndexSubstitution::index_pair_t(si1, si2));
		ct::Tensor::index_list_t sourceIndices     = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t sourceSymmetry = { ct::IndexSubstitution(sp1) };
		const ct::Tensor symmetrySource("H", sourceIndices, sourceSymmetry);

		ct::Index i1 = idx("l+");
		ct::Index i2 = idx("k+");
		ct::Index i3 = idx("j");
		ct::Index i4 = idx("m");
		ct::IndexSubstitution p1(ct::IndexSubstitution::index_pair_t(si1, si2));
		ct::Tensor::index_list_t indices       = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t symmetries = { ct::IndexSubstitution(p1) };
		const ct::Tensor expected("H", indices, symmetries);
		ct::Tensor transformed("H", indices);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		// Duplicate indices
		ct::Index si1 = idx("i+");
		ct::Index si2 = idx("i+");
		ct::Index si3 = idx("j");
		ct::Index si4 = idx("j");
		ct::IndexSubstitution sp1(ct::IndexSubstitution::index_pair_t(si1, si3));
		ct::Tensor::index_list_t sourceIndices     = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t sourceSymmetry = { ct::IndexSubstitution(sp1) };
		const ct::Tensor symmetrySource("H", sourceIndices, sourceSymmetry);

		ct::Index i1 = idx("p+");
		ct::Index i2 = idx("p+");
		ct::Index i3 = idx("j");
		ct::Index i4 = idx("l");
		ct::IndexSubstitution p1(ct::IndexSubstitution::index_pair_t(si1, si3));
		ct::Tensor::index_list_t indices       = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t symmetries = { ct::IndexSubstitution(p1) };
		const ct::Tensor expected("H", indices, symmetries);
		ct::Tensor transformed("H", indices);

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
	{
		// Destination tensor already has some symmetry (which will be overwritten)
		ct::Index si1 = idx("i+");
		ct::Index si2 = idx("j+");
		ct::Index si3 = idx("j");
		ct::Index si4 = idx("j");
		ct::IndexSubstitution sp1(ct::IndexSubstitution::index_pair_t(si1, si3));
		ct::Tensor::index_list_t sourceIndices     = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t sourceSymmetry = { ct::IndexSubstitution(sp1) };
		const ct::Tensor symmetrySource("H", sourceIndices, sourceSymmetry);

		ct::Index i1 = idx("l+");
		ct::Index i2 = idx("k+");
		ct::Index i3 = idx("j");
		ct::Index i4 = idx("m");
		ct::IndexSubstitution p1(ct::IndexSubstitution::index_pair_t(si1, si3));
		ct::Tensor::index_list_t indices       = { ct::Index(si1), ct::Index(si2), ct::Index(si3), ct::Index(si4) };
		ct::Tensor::symmetry_list_t symmetries = { ct::IndexSubstitution(p1) };
		const ct::Tensor expected("H", indices, symmetries);
		ct::Tensor transformed("H", indices,
							   { ct::IndexSubstitution({ ct::IndexSubstitution::index_pair_t(i3, i4) }) });

		ct::Tensor::transferSymmetry(symmetrySource, transformed);

		ASSERT_EQ(transformed, expected);
	}
}

TEST(TensorTest, replaceIndex) {
	{
		ct::Index index1 = idx("j+");
		ct::Index index2 = idx("k+");
		ct::Index index3 = idx("l+");

		ct::Tensor expected("H", { ct::Index(index3), ct::Index(index2) },
							{ ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index3, index2)) });

		ct::Tensor actual("H", { ct::Index(index1), ct::Index(index2) },
						  { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index1, index2)) });
		actual.replaceIndex(index1, index3);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index index1 = idx("j+");
		ct::Index index2 = idx("k+");
		ct::Index index3 = idx("l+");

		ct::Tensor expected("H", { ct::Index(index1), ct::Index(index3) },
							{ ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index1, index3)) });

		ct::Tensor actual("H", { ct::Index(index1), ct::Index(index2) },
						  { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index1, index2)) });
		actual.replaceIndex(index2, index3);

		ASSERT_EQ(actual, expected);
	}
	{
		// Replacing a non-existent index is a no-op
		ct::Index index1 = idx("j+");
		ct::Index index2 = idx("k+");
		ct::Index index3 = idx("l+");
		ct::Index dummy  = idx("m+");

		ct::Tensor expected("H", { ct::Index(index1), ct::Index(index2) },
							{ ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index1, index2)) });

		ct::Tensor actual("H", { ct::Index(index1), ct::Index(index2) },
						  { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(index1, index2)) });
		actual.replaceIndex(dummy, index3);

		ASSERT_EQ(actual, expected);
	}
}

TEST(TensorTest, replaceIndices) {
	{
		ct::Tensor expected("H", { idx("i"), idx("j") });
		ct::Tensor actual("H", { idx("j"), idx("i") });

		// Switch i and j by replacing i->j and j->i
		// Note that if we were to apply these separately using the replaceIndex function we'd end
		// up with either H[ii] or H[jj] instead of the desired H[ij]
		std::vector< std::pair< ct::Index, ct::Index > > replacements = { { idx("i"), idx("j") },
																		  { idx("j"), idx("i") } };

		actual.replaceIndices(replacements);

		ASSERT_EQ(actual, expected);
	}
	{
		// Do the same but this time include index symmetries
		ct::Tensor expected("H", { idx("i"), idx("j") },
							{ ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("j"))) });
		ct::Tensor actual("H", { idx("j"), idx("i") },
						  { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("j"), idx("i"))) });

		std::vector< std::pair< ct::Index, ct::Index > > replacements = { { idx("i"), idx("j") },
																		  { idx("j"), idx("i") } };

		actual.replaceIndices(replacements);

		ASSERT_EQ(actual, expected);
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

	ASSERT_TRUE(one.refersToSameElement(two));

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

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
	{
		// No contraction possible
		ct::Tensor t1("T1", { idx("i+") });
		ct::Tensor t2("T2", { idx("j") });

		ct::Tensor expectedResult("T1_T2", { idx("i+"), idx("j") });
		ct::ContractionResult::cost_t expectedCost = pow(occupiedSize, 2);

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
	{
		// Contraction over a single common index with other indices present as well
		ct::Tensor t1("T1", { idx("a"), idx("i+") });
		ct::Tensor t2("T2", { idx("i"), idx("b+") });

		ct::Tensor expectedResult("T1_T2", { idx("a"), idx("b+") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize * pow(virtualSize, 2);

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
	{
		// Contraction over two common indices
		ct::Tensor t1("T1", { idx("a"), idx("i+") });
		ct::Tensor t2("T2", { idx("i"), idx("b"), idx("a+") });

		ct::Tensor expectedResult("T1_T2", { idx("b") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize * pow(virtualSize, 2);

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
	{
		// "Contraction" with scalar
		ct::Tensor t1("T1", { idx("i") });
		ct::Tensor t2("T2", {});

		ct::Tensor expectedResult("T1_T2", { idx("i") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize;

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
	{
		// "Contraction" with scalar (reversed)
		ct::Tensor t1("T1", { idx("i") });
		ct::Tensor t2("T2", {});

		ct::Tensor expectedResult("T1_T2", { idx("i") });
		ct::ContractionResult::cost_t expectedCost = occupiedSize;

		ct::ContractionResult result = t2.contract(t1, resolver);

		ASSERT_EQ(result.result, expectedResult);
		ASSERT_EQ(result.cost, expectedCost);
	}
}

TEST(TensorTest, contractWithSymmetry) {
	{
		// A_B[ij,ab] = A[i,a,q] B[j,b,q]
		ct::IndexSubstitution sym1({ idx("i"), idx("a") }, 1);
		ct::IndexSubstitution sym2({ idx("j"), idx("b") }, 1);

		ct::Tensor t1("A", { idx("i+"), idx("a"), idx("q!") }, { sym1 });
		ct::Tensor t2("B", { idx("j+"), idx("b"), idx("q!") }, { sym2 });

		ct::Tensor expectedResult("A_B", { idx("i+"), idx("j+"), idx("a"), idx("b") }, { sym1, sym2 });

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
	}
	{
		// A_B[j,b] = A[ij,ab] B[a,i]
		ct::IndexSubstitution sym1({ idx("i"), idx("a") }, 1);
		ct::IndexSubstitution sym2({ idx("j"), idx("b") }, 1);
		ct::IndexSubstitution sym3({ idx("a"), idx("i") }, 1);

		ct::Tensor t1("A", { idx("i+"), idx("j+"), idx("a"), idx("b") }, { sym1, sym2 });
		ct::Tensor t2("B", { idx("a+"), idx("i") }, { sym3 });

		ct::Tensor expectedResult("A_B", { idx("j+"), idx("b") }, { sym2 });

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
	}
	{
		// A_B[j,b] = A[ij,ab] B[a,i]
		ct::IndexSubstitution sym1({ idx("i"), idx("b") }, -1);

		ct::Tensor t1("A", { idx("i+"), idx("j+"), idx("a"), idx("b") }, { sym1 });
		ct::Tensor t2("B", { idx("a+"), idx("i") });

		ct::Tensor expectedResult("A_B", { idx("j+"), idx("b") }, {});

		ct::ContractionResult result = t1.contract(t2, resolver);

		ASSERT_EQ(result.result, expectedResult);
	}
}
