#include "utils/Iterable.hpp"

#include <functional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

class Dummy {
public:
	static constexpr std::size_t SIZE = 10;
	using container_t                 = std::vector< std::string >;

	Dummy() {
		for (std::size_t i = 0; i < SIZE; i++) {
			m_vec.push_back(std::to_string(i));
		}
	}

	Contractor::Iterable< std::string > objects() {
		std::function< std::string &(std::size_t) > func =
			std::bind(static_cast< std::string &(container_t::*) (std::size_t) >(&container_t::at), &m_vec,
					  std::placeholders::_1);

		return Contractor::Iterable< std::string >(0, m_vec.size(), func, &m_vec);
	}

	Contractor::Iterable< const std::string > objects() const {
		auto temp = std::bind(static_cast< const std::string &(container_t::*) (std::size_t) const >(&container_t::at),
							  &m_vec, std::placeholders::_1);

		std::function< const std::string &(std::size_t) > func = temp;

		return Contractor::Iterable< const std::string >(0, m_vec.size(), func, &m_vec);
	}

protected:
	std::vector< std::string > m_vec;
};

TEST(IteratorTest, iterable) {
	Dummy dummy;

	auto it = dummy.objects().begin();
	for (std::size_t i = 0; i < Dummy::SIZE; i++) {
		ASSERT_EQ(*it, std::to_string(i));
		++it;
	}
}

TEST(IteratorTest, const_iterable) {
	const Dummy dummy;

	auto it = dummy.objects().begin();
	for (std::size_t i = 0; i < Dummy::SIZE; i++) {
		ASSERT_EQ(*it, std::to_string(i));
		++it;
	}
}

TEST(IteratorTest, ranged_for) {
	Dummy dummy;

	std::size_t index = 0;
	for (std::string &current : dummy.objects()) {
		ASSERT_EQ(current, std::to_string(index));
		index++;
	}
}

TEST(IteratorTest, const_ranged_for) {
	const Dummy dummy;

	std::size_t index = 0;
	for (const std::string &current : dummy.objects()) {
		ASSERT_EQ(current, std::to_string(index));
		index++;
	}
}

TEST(IteratorTest, iterator_compare) {
	const Dummy dummy1;
	const Dummy dummy2;

	auto iterable1 = dummy1.objects();
	auto iterable2 = dummy2.objects();

	ASSERT_EQ(iterable1.begin(), iterable1.begin());
	ASSERT_EQ(iterable1.end(), iterable1.end());
	ASSERT_EQ(iterable1.cbegin(), iterable1.cbegin());
	ASSERT_EQ(iterable1.cend(), iterable1.cend());

	ASSERT_NE(iterable1.begin(), iterable2.begin());
	ASSERT_NE(iterable1.end(), iterable2.end());
	ASSERT_NE(iterable1.cbegin(), iterable2.cbegin());
	ASSERT_NE(iterable1.cend(), iterable2.cend());

	ASSERT_NE(iterable2.begin(), iterable1.begin());
	ASSERT_NE(iterable2.end(), iterable1.end());
	ASSERT_NE(iterable2.cbegin(), iterable1.cbegin());
	ASSERT_NE(iterable2.cend(), iterable1.cend());

	auto it = iterable1.begin();
	ASSERT_EQ(it, iterable1.begin());

	for (std::size_t i = 0; i < Dummy::SIZE; i++) {
		++it;
	}

	ASSERT_EQ(it, iterable1.end());
}
