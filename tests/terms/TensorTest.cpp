#include "terms/Tensor.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(TensorTest, getter) {
	ct::Tensor empty1("H");
	ct::Tensor empty2("T2");

	ASSERT_EQ(empty1.getName(), "H");
	ASSERT_EQ(empty2.getName(), "T2");
	ASSERT_EQ(empty1.indices().size(), 0);
	ASSERT_EQ(empty2.indices().size(), 0);

	ct::Tensor::index_list_t creators;
	creators.push_back(ct::Index::occupiedIndex(1, true, ct::Index::Type::Creator));
	creators.push_back(ct::Index::occupiedIndex(3, true, ct::Index::Type::Creator));
	creators.push_back(ct::Index::virtualIndex(0, true, ct::Index::Type::Creator));
	creators.push_back(ct::Index::virtualIndex(3, true, ct::Index::Type::Creator));

	ct::Tensor creatorsOnly("H", creators);

	ASSERT_EQ(creatorsOnly.getName(), "H");
	ASSERT_EQ(creatorsOnly.copyIndices(), creators);

	ct::Tensor::index_list_t annihilators;
	annihilators.push_back(ct::Index::virtualIndex(0, true, ct::Index::Type::Annihilator));
	annihilators.push_back(ct::Index::occupiedIndex(2, true, ct::Index::Type::Annihilator));
	annihilators.push_back(ct::Index::occupiedIndex(6, true, ct::Index::Type::Annihilator));
	annihilators.push_back(ct::Index::virtualIndex(2, true, ct::Index::Type::Annihilator));

	ct::Tensor annihilatorsOnly("H", annihilators);

	ASSERT_EQ(annihilatorsOnly.getName(), "H");
	ASSERT_EQ(annihilatorsOnly.copyIndices(), annihilators);
}

ct::Index createIndex(ct::Index::id_t id, bool occupied) {
	if (occupied) {
		return ct::Index::occupiedIndex(id, true, ct::Index::Type::None);
	} else {
		return ct::Index::virtualIndex(id, true, ct::Index::Type::None);
	}
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
		ct::Tensor element1("H", {createIndex(0, true)});
		ct::Tensor element2("H");
		ct::Tensor element3("H", {createIndex(0, true)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {createIndex(0, true)});
		ct::Tensor element2("H", {createIndex(1, true)});
		ct::Tensor element3("H", {createIndex(0, true)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {createIndex(0, true)});
		ct::Tensor element2("H", {createIndex(0, false)});
		ct::Tensor element3("H", {createIndex(0, true)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}
}

ct::Tensor createTensor(const std::string_view name, std::size_t indexCount) {
	ct::Tensor::index_list_t indices;

	for (std::size_t i = 0; i < indexCount; i++) {
		indices.push_back(ct::Index(ct::IndexSpace(i % 5), i, true, ct::Index::Type::None));
	}

	return ct::Tensor(name, std::move(indices));
}

TEST(TensorTest, helperFunction) {
	ct::Tensor element = createTensor("ABC", 3);

	ASSERT_EQ(element.getName(), "ABC");
	ASSERT_EQ(element.indices().size(), 3);
}

TEST(TensorTest, copy) {
	ct::Tensor element = createTensor("H", 3);
	ct::Tensor copy = ct::Tensor(element);

	ASSERT_EQ(element, copy);
}

TEST(TensorTest, move) {
	ct::Tensor element = createTensor("H", 4);
	ct::Tensor copy = ct::Tensor(element);

	ct::Tensor newTensor = std::move(element);

	// After the move the new element should be what element was before the move whilst element
	// is now basically an empty hull
	ASSERT_EQ(newTensor, copy);
	ASSERT_EQ(element.getName(), "");
	ASSERT_EQ(element.indices().size(), 0);
}
