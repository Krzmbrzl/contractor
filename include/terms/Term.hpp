#ifndef CONTRACTOR_TERMS_TERM_HPP_
#define CONTRACTOR_TERMS_TERM_HPP_

#include "terms/Tensor.hpp"
#include "utils/Iterable.hpp"

#include <cstdint>
#include <ostream>
#include <type_traits>

namespace Contractor::Terms {

/**
 * A class representing a "term". A term usually describes a particular diagram that can be created after
 * having applied Wick's theorem to an expectation value formulated in second quantization.
 * More generally speaking a Term is a container for a product of Tensors with a given prefactor.
 *
 * A Term always has a Tensor as its parent. This parent describes the Tensor to which this Term contributes
 * (once the actual numerics is being carried out). Multiple Terms with the same parent are understood to
 * be all added to that parent Tensor.
 */
class Term {
public:
	/**
	 * The type used to represent the pre-factor of a term
	 */
	using factor_t = float;

	/**
	 * Different available options that can be used when using Term::equals
	 */
	struct CompareOption {
		// The struct is necessary to serve as a namespace since plain enums don't provide that
		enum Options : uint8_t {
			NONE               = 0b00000000,
			REQUIRE_SAME_ORDER = 0b00000001,
			REQUIRE_SAME_TYPE  = 0b00000010,
		};
	};

	explicit Term(const Tensor &result = Tensor(), factor_t prefactor = {});
	Term(const Term &) = default;
	Term(Term &&)      = default;
	virtual ~Term()    = default;
	Term &operator=(const Term &other) = default;
	Term &operator=(Term &&other) = default;

	friend bool operator==(const Term &lhs, const Term &rhs);
	friend bool operator!=(const Term &lhs, const Term &rhs);

	friend std::ostream &operator<<(std::ostream &stream, const Term &term);

	/**
	 * @returns The result Tensor this Term contributes to
	 */
	const Tensor &getResult() const;
	/**
	 * @param result The new result Tensor to use
	 */
	void setResult(const Tensor &result);

	/**
	 * @returns A mutable reference to the result Tensor this Term contributes to
	 */
	Tensor &accessResult();

	/**
	 * @returns The prefactor of this Term
	 */
	factor_t getPrefactor() const;
	/**
	 * @param prefactor The new prefactor to use
	 */
	void setPrefactor(factor_t prefactor);

	/**
	 * @returns The size of this Term. The size equals the amount of Tensors contained in this Term.
	 */
	virtual std::size_t size() const = 0;
	/**
	 * @returns An object that can be iterated over in order to visit the different Tensors contained
	 * in this term
	 */
	Iterable< const Tensor > getTensors() const;

	/**
	 * @returns An object that can be iterated over in order to visit the different Tensors contained
	 * in this term
	 */
	Iterable< Tensor > accessTensors();

	/**
	 * @param other The Term to compare with
	 * @param options The options to use when performing the comparison. The different options are specified
	 * in the CompareOption namespace and can be combined using bitwise AND.
	 * @returns Whether this Term and the given one are considered equal under the application of the given
	 * options
	 */
	virtual bool equals(const Term &other,
						std::underlying_type_t< CompareOption::Options > options = CompareOption::NONE) const;

	/**
	 * Deduces the symmetry of the result Tensor of this Term based on the Tensors contained
	 * in the Term (and their symmetry).
	 */
	void deduceSymmetry();

protected:
	Tensor m_result;
	factor_t m_prefactor;

	/**
	 * @returns A reference to the Tensor at the given index. This helper function is used for making
	 * iterating over Terms possible.
	 */
	virtual Tensor &get(std::size_t index) = 0;

	/**
	 * @returns A reference to the Tensor at the given index. This helper function is used for making
	 * iterating over Terms possible.
	 */
	virtual const Tensor &get(std::size_t index) const = 0;
};


}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TERM_HPP_
