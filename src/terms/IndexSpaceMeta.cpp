#include "terms/IndexSpaceMeta.hpp"

namespace Contractor::Terms {
IndexSpace::id_t IndexSpaceMeta::s_nextID = 0;

bool operator==(const IndexSpaceMeta &lhs, const IndexSpaceMeta &rhs) {
	return lhs.m_name == rhs.m_name && lhs.m_label == rhs.m_label && lhs.m_size == rhs.m_size
		   && lhs.m_space == rhs.m_space;
}

bool operator!=(const IndexSpaceMeta &lhs, const IndexSpaceMeta &rhs) {
	return !(lhs == rhs);
}

IndexSpaceMeta::IndexSpaceMeta(const IndexSpaceMeta::name_t &name, IndexSpaceMeta::label_t label,
							   IndexSpaceMeta::size_t size, Index::Spin defaultSpin)
	: m_name(name), m_label(label), m_size(size), m_space(s_nextID++), m_defaultSpin(defaultSpin) {
}

IndexSpaceMeta::IndexSpaceMeta(IndexSpaceMeta::name_t &&name, IndexSpaceMeta::label_t label,
							   IndexSpaceMeta::size_t size, Index::Spin defaultSpin)
	: m_name(name), m_label(label), m_size(size), m_space(s_nextID++), m_defaultSpin(defaultSpin) {
}

const std::string &IndexSpaceMeta::getName() const {
	return m_name;
}

char IndexSpaceMeta::getLabel() const {
	return m_label;
}

unsigned int IndexSpaceMeta::getSize() const {
	return m_size;
}

const IndexSpace &IndexSpaceMeta::getSpace() const {
	return m_space;
}

Index::Spin IndexSpaceMeta::getDefaultSpin() const {
	return m_defaultSpin;
}

}; // namespace Contractor::Terms
