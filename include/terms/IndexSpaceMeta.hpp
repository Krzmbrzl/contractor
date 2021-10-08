#ifndef CONTRACTOR_TERMS_INDEXSPACEMETA_HPP_
#define CONTRACTOR_TERMS_INDEXSPACEMETA_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"

#include <string>

namespace Contractor::Terms {

/**
 * Class intended to hold meta information about an IndexSpace
 */
class IndexSpaceMeta {
public:
	/**
	 * Type used for storing the name of an index space
	 */
	using name_t = std::string;
	/**
	 * Type used for storing the label of an index space
	 */
	using label_t = char;
	/**
	 * Type used for storing the size of an index space
	 */
	using size_t = unsigned int;

	explicit IndexSpaceMeta(const name_t &name, label_t label, size_t size, Index::Spin defaultSpin);
	explicit IndexSpaceMeta(name_t &&name, label_t label, size_t size, Index::Spin defaultSpin);
	~IndexSpaceMeta() = default;

	explicit IndexSpaceMeta(const IndexSpaceMeta &) = default;
	IndexSpaceMeta(IndexSpaceMeta &&)               = default;
	IndexSpaceMeta &operator=(const IndexSpaceMeta &) = default;
	IndexSpaceMeta &operator=(IndexSpaceMeta &&) = default;

	friend bool operator==(const IndexSpaceMeta &lhs, const IndexSpaceMeta &rhs);
	friend bool operator!=(const IndexSpaceMeta &lhs, const IndexSpaceMeta &rhs);

	/**
	 * @returns The canonical name of the associated space
	 */
	const std::string &getName() const;
	/**
	 * @returns The 1-char label for the associated space
	 */
	char getLabel() const;
	/**
	 * @returns The (relative) size of this space. The size is a measure of the amount of distinct
	 * indices the associated space runs over.
	 */
	unsigned int getSize() const;
	/**
	 * @returns An instance of the associated IndexSpace
	 */
	const IndexSpace &getSpace() const;
	/**
	 * @returns The default Spin indices in this space are supposed to have
	 */
	Index::Spin getDefaultSpin() const;

protected:
	static IndexSpace::id_t s_nextID;

	std::string m_name;
	char m_label;
	unsigned int m_size;
	IndexSpace m_space;
	Index::Spin m_defaultSpin;
};
}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEXSPACEMETA_HPP_
