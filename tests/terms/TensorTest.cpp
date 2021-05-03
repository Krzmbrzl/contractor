#include "terms/Tensor.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(TensorTest, getter) {
	ct::Tensor empty1("H");
	ct::Tensor empty2("T2");

	ASSERT_EQ(empty1.getName(), "H");
	ASSERT_EQ(empty2.getName(), "T2");
	ASSERT_EQ(empty1.creatorIndices().size(), 0);
	ASSERT_EQ(empty2.creatorIndices().size(), 0);
	ASSERT_EQ(empty1.annihilatorIndices().size(), 0);
	ASSERT_EQ(empty2.annihilatorIndices().size(), 0);

	ct::Tensor::index_list_t creators;
	creators.push_back(ct::Index::occupiedIndex(1));
	creators.push_back(ct::Index::occupiedIndex(3));
	creators.push_back(ct::Index::virtualIndex(0));
	creators.push_back(ct::Index::virtualIndex(3));

	ct::Tensor creatorsOnly("H", creators);

	ASSERT_EQ(creatorsOnly.getName(), "H");
	ASSERT_EQ(creatorsOnly.copyCreatorIndices(), creators);
	ASSERT_EQ(creatorsOnly.annihilatorIndices().size(), 0);

	ct::Tensor::index_list_t annihilators;
	annihilators.push_back(ct::Index::virtualIndex(0));
	annihilators.push_back(ct::Index::occupiedIndex(2));
	annihilators.push_back(ct::Index::occupiedIndex(6));
	annihilators.push_back(ct::Index::virtualIndex(2));

	ct::Tensor annihilatorsOnly("H", {}, annihilators);

	ASSERT_EQ(annihilatorsOnly.getName(), "H");
	ASSERT_EQ(annihilatorsOnly.creatorIndices().size(), 0);
	ASSERT_EQ(annihilatorsOnly.copyAnnihilatorIndices(), annihilators);

	ct::Tensor both("H", creators, annihilators);

	ASSERT_EQ(both.getName(), "H");
	ASSERT_EQ(both.copyCreatorIndices(), creators);
	ASSERT_EQ(both.copyAnnihilatorIndices(), annihilators);

	// If this check doesn't hold we can't be sure that the both object didn't mix up the indices
	ASSERT_NE(creators, annihilators);
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
		ct::Tensor element1("H", {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H");
		ct::Tensor element3("H", {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {ct::Index::occupiedIndex(1)});
		ct::Tensor element3("H", {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {ct::Index::virtualIndex(0)});
		ct::Tensor element3("H", {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H");
		ct::Tensor element3("H", {}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {}, {ct::Index::occupiedIndex(1)});
		ct::Tensor element3("H", {}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {}, {ct::Index::virtualIndex(0)});
		ct::Tensor element3("H", {}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {}, {ct::Index::virtualIndex(0)});
		ct::Tensor element3("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {ct::Index::occupiedIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element3("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H", {ct::Index::virtualIndex(2)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element3("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}

	{
		ct::Tensor element1("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element2("H1", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});
		ct::Tensor element3("H", {ct::Index::virtualIndex(1)}, {ct::Index::occupiedIndex(0)});

		ASSERT_NE(element1, element2);
		ASSERT_EQ(element1, element3);
	}
}

ct::Tensor createTensor(const std::string_view name, std::size_t creatorCount, std::size_t annihilatorCount) {
	ct::Tensor::index_list_t creators;
	ct::Tensor::index_list_t annihilators;

	for (std::size_t i = 0; i < creatorCount; i++) {
		creators.push_back(ct::Index(ct::IndexSpace(i % 5), i));
	}

	for (std::size_t i = 0; i < annihilatorCount; i++) {
		annihilators.push_back(ct::Index(ct::IndexSpace((i + 1) % 5), i));
	}

	return ct::Tensor(name, std::move(creators), std::move(annihilators));
}

TEST(TensorTest, helperFunction) {
	ct::Tensor element = createTensor("ABC", 3, 3);

	ASSERT_EQ(element.getName(), "ABC");
	ASSERT_EQ(element.creatorIndices().size(), 3);
	ASSERT_EQ(element.annihilatorIndices().size(), 3);
	ASSERT_NE(element.copyCreatorIndices(), element.copyAnnihilatorIndices());
}

TEST(TensorTest, copy) {
	ct::Tensor element = createTensor("H", 3, 4);
	ct::Tensor copy = ct::Tensor(element);

	ASSERT_EQ(element, copy);
}

TEST(TensorTest, move) {
	ct::Tensor element = createTensor("H", 3, 4);
	ct::Tensor copy = ct::Tensor(element);

	ct::Tensor newTensor = std::move(element);

	// After the move the new element should be what element was before the move whilst element
	// is now basically an empty hull
	ASSERT_EQ(newTensor, copy);
	ASSERT_EQ(element.getName(), "");
	ASSERT_EQ(element.creatorIndices().size(), 0);
	ASSERT_EQ(element.annihilatorIndices().size(), 0);
}
