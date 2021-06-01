#ifndef CONTRACTOR_PROCESSOR_FACTORIZATION_HPP_
#define CONTRACTOR_PROCESSOR_FACTORIZATION_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp" // ContractionResult

#include <vector>

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Processor {

/**
 * Computes the optimal factorization into BinaryTerm objects for the given term. The optimal factorization
 * is the one with the least cost where cost equals amount of iterations that need to be performed in the
 * contraction of the individual Tensors.
 *
 * @param term The GeneralTerm to factorize
 * @param resolver The IndexSpaceResolver to use for accessing the index space sizes
 * @param factorizationCost A pointer to a variable in which the cost of the found factorization is to be
 * stored. This may be nullptr if this information is not needed.
 * @returns A list of BinaryTerm objects that represent the optimal factorization.
 */
std::vector< Terms::BinaryTerm > factorize(const Terms::GeneralTerm &term, const Utils::IndexSpaceResolver &resolver,
										   Terms::ContractionResult::cost_t *factorizationCost = nullptr);

}; // namespace Contractor::Processor

#endif // CONTRACTOR_PROCESSOR_FACTORIZATION_HPP_
