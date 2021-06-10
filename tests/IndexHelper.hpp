#ifndef CONTRACTOR_TESTS_INDEX_HELPER_HPP_
#define CONTRACTOR_TESTS_INDEX_HELPER_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <stdexcept>
#include <string_view>

static Contractor::Utils::IndexSpaceResolver resolver({
	Contractor::Terms::IndexSpaceMeta("occupied", 'H', 10, Contractor::Terms::Index::Spin::Both),
	Contractor::Terms::IndexSpaceMeta("virtual", 'P', 100, Contractor::Terms::Index::Spin::Both),
	Contractor::Terms::IndexSpaceMeta("external", 'Q', 80, Contractor::Terms::Index::Spin::None),
});


static Contractor::Terms::Index idx(const std::string_view spec) {
	if (spec.size() > 2) {
		throw std::runtime_error("Index spec is too large");
	}
	if (spec.size() < 1) {
		throw std::runtime_error("Index spec is too short");
	}

	char baseChar;
	Contractor::Terms::IndexSpace indexSpace;
	Contractor::Terms::Index::Spin indexSpin;
	char indexName = spec[0];

	if (indexName >= 'a' && indexName <= 'h') {
		indexSpace = resolver.resolve("virtual");
		baseChar   = 'a';
	} else if (indexName >= 'i' && indexName <= 'p') {
		indexSpace = resolver.resolve("occupied");
		baseChar   = 'i';
	} else if (indexName >= 'q' && indexName <= 'x') {
		indexSpace = resolver.resolve("external");
		baseChar   = 'q';
	} else {
		throw std::runtime_error("Unknown index name in spec");
	}

	indexSpin = resolver.getMeta(indexSpace).getDefaultSpin();

	Contractor::Terms::Index::Type indexType = Contractor::Terms::Index::Type::Annihilator;
	if (spec.size() == 2) {
		switch (spec[1]) {
			case '+':
				indexType = Contractor::Terms::Index::Type::Creator;
				break;
			case '!':
				indexType = Contractor::Terms::Index::Type::None;
				break;
			default:
				throw std::runtime_error("Invalid index modifier");
		}
	}

	return Contractor::Terms::Index(indexSpace, indexName - baseChar, indexType, indexSpin);
}

#endif // CONTRACTOR_TESTS_INDEX_HELPER_HPP_
