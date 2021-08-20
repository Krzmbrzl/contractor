#ifndef CONTRACTOR_UTILS_INDEXSPACE_RESOLVER_HPP_
#define CONTRACTOR_UTILS_INDEXSPACE_RESOLVER_HPP_

#include "terms/IndexSpaceMeta.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Contractor::Terms {
class IndexSpace;
};

namespace Contractor::Utils {

/**
 * Exception thrown when resolving failed
 */
class ResolveException : public std::exception {
public:
	ResolveException(const std::string_view msg = "");

	const char *what() const noexcept override;

protected:
	std::string m_msg;
};


/**
 * This class keeps track of several IndexSpaceMeta instances and uses them to resolve IndexSpace
 * instances from their label or name.
 */
class IndexSpaceResolver {
public:
	/**
	 * Type used for storing the IndexSpaceMeta instances
	 */
	using meta_list_t = std::vector< Terms::IndexSpaceMeta >;

	IndexSpaceResolver(const meta_list_t &list = {});
	IndexSpaceResolver(meta_list_t &&list);

	// If these types are the same, we can't overload our functions properly
	static_assert(!std::is_same_v< Terms::IndexSpaceMeta::label_t, Terms::IndexSpaceMeta::name_t >);
	/**
	 * @param label The label for the index space to search for
	 * @returns An IndexSpace instace corresponding to the given label
	 *
	 * @throws ResolveException if resolving fails
	 */
	Terms::IndexSpace resolve(Terms::IndexSpaceMeta::label_t label) const;
	/**
	 * @param name The name of the index space to search for
	 * @returns An IndexSpace instace corresponding to the given name
	 *
	 * @throws ResolveException if resolving fails
	 */
	Terms::IndexSpace resolve(const Terms::IndexSpaceMeta::name_t &name) const;

	/**
	 * @param space The IndexSpace object whose corresponding meta object shall be obtained
	 * @returned The IndexSpaceMeta object corresponding to the given space
	 */
	const Terms::IndexSpaceMeta &getMeta(const Terms::IndexSpace &space) const;

	/**
	 * @returns The list of meta spaces managed by this resolver
	 */
	const meta_list_t &getMetaList() const;

	/**
	 * @param label The label for the index space to search for
	 * @returns Whether this resolver contains an index space identified by the given label
	 */
	bool contains(const Terms::IndexSpaceMeta::label_t &label) const;

	/**
	 * @param name The name for the index space to search for
	 * @returns Whether this resolver contains an index space identified by the given name
	 */
	bool contains(const Terms::IndexSpaceMeta::name_t &name) const;

protected:
	meta_list_t m_list;
};

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_INDEXSPACE_RESOLVER_HPP_
