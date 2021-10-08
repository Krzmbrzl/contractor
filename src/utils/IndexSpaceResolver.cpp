#include "utils/IndexSpaceResolver.hpp"
#include "terms/IndexSpace.hpp"
#include "utils/Utils.hpp"

namespace Contractor::Utils {

ResolveException::ResolveException(const std::string_view msg) : m_msg(msg) {
}

const char *ResolveException::what() const noexcept {
	return m_msg.c_str();
}

IndexSpaceResolver::IndexSpaceResolver(const IndexSpaceResolver::meta_list_t &list) : m_list(list) {
}

IndexSpaceResolver::IndexSpaceResolver(IndexSpaceResolver::meta_list_t &&list) : m_list(list) {
}

Terms::IndexSpace IndexSpaceResolver::resolve(Terms::IndexSpaceMeta::label_t label) const {
	for (const Terms::IndexSpaceMeta &currentMeta : m_list) {
		if (label == currentMeta.getLabel()) {
			return currentMeta.getSpace();
		}
	}

	throw ResolveException(std::string("Unknown label for index space \"") + label + "\"");
}

Terms::IndexSpace IndexSpaceResolver::resolve(const Terms::IndexSpaceMeta::name_t &name) const {
	for (const Terms::IndexSpaceMeta &currentMeta : m_list) {
		if (name == currentMeta.getName()) {
			return currentMeta.getSpace();
		}
	}

	throw ResolveException(std::string("Unknown name for index space \"") + name + "\"");
}

const Terms::IndexSpaceMeta &IndexSpaceResolver::getMeta(const Terms::IndexSpace &space) const {
	for (const Terms::IndexSpaceMeta &currentMeta : m_list) {
		if (space == currentMeta.getSpace()) {
			return currentMeta;
		}
	}

	throw ResolveException(std::string("Unknown index space \"") + to_string(space) + "\"");
}

const IndexSpaceResolver::meta_list_t &IndexSpaceResolver::getMetaList() const {
	return m_list;
}

bool IndexSpaceResolver::contains(const Terms::IndexSpaceMeta::label_t &label) const {
	for (const Terms::IndexSpaceMeta &currentMeta : m_list) {
		if (label == currentMeta.getLabel()) {
			return true;
		}
	}

	return false;
}

bool IndexSpaceResolver::contains(const Terms::IndexSpaceMeta::name_t &name) const {
	for (const Terms::IndexSpaceMeta &currentMeta : m_list) {
		if (name == currentMeta.getName()) {
			return true;
		}
	}

	return false;
}

}; // namespace Contractor::Utils
