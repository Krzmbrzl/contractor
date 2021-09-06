#include "formatting/ITFExporter.hpp"

#include "terms/Index.hpp"
#include "terms/PermutationGroup.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

namespace Contractor::Formatting {

ITFExporter::ITFExporter(const cu::IndexSpaceResolver &resolver, std::ostream &sink, std::string_view codeBlock,
						 const Predicate &isIntermediate)
	: m_sink(nullptr), m_codeBlock(codeBlock), m_resolver(resolver), m_isIntermediate(isIntermediate) {
	setSink(sink);
}

ITFExporter::~ITFExporter() {
	if (m_sink) {
		m_sink->flush();
	}
}

void ITFExporter::addComposites(const std::vector< ct::BinaryCompositeTerm > &composites) {
	addComposites(composites.begin(), composites.end());
}

void ITFExporter::addComposite(const ct::BinaryCompositeTerm &composite) {
	assert(m_sink != nullptr);

	*m_sink << "BEGIN\n";

	for (const ct::BinaryTerm &currentTerm : composite) {
		writeTerm(currentTerm);
	}

	*m_sink << "END\n";
}

void ITFExporter::setSink(std::ostream &sink) {
	m_sink = &sink;

	*m_sink << "CODE_BLOCK: " << m_codeBlock << "\n";
}

void ITFExporter::writeTerm(const ct::BinaryTerm &term) {
	assert(m_sink != nullptr);

	// Result Tensor is marked with a preceding dot
	*m_sink << ".";
	writeTensor(term.getResult());

	*m_sink << " ";
	if (term.getPrefactor() < 0) {
		*m_sink << "-= ";
	} else {
		*m_sink << "+= ";
	}

	if (std::abs(term.getPrefactor()) != 1) {
		*m_sink << std::abs(term.getPrefactor()) << "*";
	}

	for (const ct::Tensor &currentTensor : term.getTensors()) {
		writeTensor(currentTensor);

		*m_sink << " ";
	}

	*m_sink << "\n";
}

std::string getTensorName(const ct::Tensor &tensor) {
	if (tensor.getIndices().size() == 0) {
		if (boost::starts_with(tensor.getName(), "ECC")) {
			// We assume this is the energy expression that - depending on the context - may be named e.g ECCD or ECCSD
			// However in ITF the energy is always expected to be named "ECC"
			return "ECC";
		}
		if (tensor.getName() == "H") {
			// reference energy
			return "Eref";
		}
	} else if (tensor.getIndices().size() == 2) {
		if (tensor.getName() == "H") {
			// 2-index H is the Fock matrix
			return "f";
		}
		if (tensor.getName() == "O1") {
			// singles residue ("O" since there is no ASCII omega which is usually used in the actual formulas)
			return "R1";
		}
	} else if (tensor.getIndices().size() == 4) {
		if (tensor.getName() == "O2") {
			// Doubles residue (again: "O" as a replacement for omega)
			return "R2";
		}
	}

	// No special Tensor -> return name as-is
	return std::string(tensor.getName());
}

void ITFExporter::writeTensor(const ct::Tensor &tensor) {
	assert(m_sink != nullptr);

	if (writeSpecialTensor(tensor)) {
		// This Tensor was treated special - thus this method is not responsible for writing it
		return;
	}

	// Sort indices such that the ones belonging to the biggest index space are located to the left. We assume that the
	// Tensors have been renamed in such a way that this reordering of indices does not cause change ambiguity as to
	// which Tensor is being referenced.
	std::vector< std::reference_wrapper< const ct::Index > > indices(tensor.getIndices().begin(),
																	 tensor.getIndices().end());
	std::vector< std::reference_wrapper< const ct::Index > > originalIndices = indices;
	std::stable_sort(indices.begin(), indices.end(), [this](const ct::Index &left, const ct::Index &right) {
		return m_resolver.getMeta(left.getSpace()).getSize() > m_resolver.getMeta(right.getSpace()).getSize();
	});

	std::string spinString         = getSpinString(indices);
	std::string originalSpinString = getSpinString(originalIndices);

	std::string printName = getTensorName(tensor);

	if (m_isIntermediate(tensor.getName())) {
		// Mark intermediates (STIN = "short-term-intermediate")
		printName += "_STIN";
	}
	if (!spinString.empty()) {
		// Mark explicit spin cases (before and after sorting)
		printName += "_" + originalSpinString + "_" + spinString;
	}

	writeTensorName(printName);

	writeIndexSequence(indices);
}

bool isSkeletonTensor(const ct::Tensor &tensor) {
	for (const ct::Index &currentIndex : tensor.getIndices()) {
		if (currentIndex.getSpin() != ct::Index::Spin::None) {
			return false;
		}
	}

	return true;
}

bool isIndexPattern(const ct::Tensor::index_list_t &indices, const std::vector< ct::IndexSpace > &spaces) {
	assert(indices.size() == spaces.size());

	for (std::size_t i = 0; i < indices.size(); ++i) {
		if (indices[i].getSpace() != spaces[i]) {
			return false;
		}
	}

	return true;
}

std::vector< ct::Index > getITFCanonicalIndexSequence(const std::vector< ct::PermutationGroup::Element > &sequences,
													  const cu::IndexSpaceResolver &resolver) {
	auto canonicalElementIt = std::min_element(
		sequences.begin(), sequences.end(),
		[resolver](const ct::PermutationGroup::Element &lhs, const Terms::PermutationGroup::Element &rhs) {
			assert(lhs.indexSequence.size() == rhs.indexSequence.size());

			for (std::size_t i = 0; i < lhs.indexSequence.size(); ++i) {
				if (lhs.indexSequence[i].getSpace() != rhs.indexSequence[i].getSpace()) {
					return resolver.getMeta(lhs.indexSequence[i].getSpace()).getSize()
						   > resolver.getMeta(rhs.indexSequence[i].getSpace()).getSize();
				}
			}

			// If the spaces of all indices are equal, then compare the usual way
			return lhs.indexSequence < rhs.indexSequence;
		});

	assert(canonicalElementIt != sequences.end());

	return canonicalElementIt->indexSequence;
}

bool ITFExporter::writeSpecialTensor(const ct::Tensor &tensor) {
	if (tensor.getName() == "H" && tensor.getIndices().size() == 4) {
		// The 4-index H-Tensor represents 2-electron integrals. This one is special since it will either be
		// mapped to the K or the J Tensor within Molpro. Note that we expect H to be a skeleton (spin-free)
		// Tensor at this point.
		//
		// The mapping is:
		// - H[abij] = K[abij]
		// - H[aibj] = J[abij]
		if (!isSkeletonTensor(tensor)) {
			throw std::runtime_error(
				"ITFExporter: Expected all 4-index H-tensors to be skeleton Tensors at this point");
		}

		const ct::IndexSpace occ  = m_resolver.resolve("occupied");
		const ct::IndexSpace virt = m_resolver.resolve("virtual");

		const ct::Tensor::index_list_t &indices = tensor.getIndices();

		// Since the special symmetry of 2-electron integrals (H[abij] = H[ijab]) can not yet be used due to
		// shortcomings of the current implementation (mostly during spin-summation & -integration), we have
		// to setup the symmetry here manually.
		// The skeleton-Tensors of the 2-electron integrals have the same symmetry as the original 2-electron-
		// integrals before they were antisymmetrized. That means that we have
		// [pqrs] = [rqps] = [psrq] = [rspq] = [qprs] = [qsrp] = [rpqs] = [rsqp]
		// In other words we have a PermutationGroup with the generators
		// - indices[0] <-> indices[2]
		// - indices[1] <-> indices[3]
		// - indices[0] <-> indices[1] & indices[2] <-> indices[3]
		ct::PermutationGroup indexGroup(tensor.getIndices());
		indexGroup.addGenerator(ct::IndexSubstitution::createPermutation({ { indices[0], indices[2] } }));
		indexGroup.addGenerator(ct::IndexSubstitution::createPermutation({ { indices[1], indices[3] } }));
		indexGroup.addGenerator(
			ct::IndexSubstitution::createPermutation({ { indices[0], indices[1] }, { indices[2], indices[3] } }));

		assert(indexGroup.size() == 8);


		std::string printName;
		std::vector< std::reference_wrapper< const ct::Index > > targetIndices;

		const std::vector< ct::Index > referenceSequence =
			getITFCanonicalIndexSequence(indexGroup.getIndexPermutations(), m_resolver);

		// Note that once we take symmetry into account, there are only the two unique possible cases of
		// the form H[abij] and H[aibj] in case of 2 occupied and 2 virtual indices.
		if (isIndexPattern(referenceSequence, { virt, virt, occ, occ })
			|| isIndexPattern(referenceSequence, { occ, occ, occ, occ })
			|| isIndexPattern(referenceSequence, { virt, virt, virt, virt })
			|| isIndexPattern(referenceSequence, { virt, occ, occ, occ })) {
			// H[abij] -> K[abij]
			// H[ijkl] -> K[ijkl]
			// H[abcd] -> K[abcd]
			// H[aijk] -> K[aijk]
			printName = "K";
			// Indices can be used in the same order they are already
			targetIndices.insert(targetIndices.end(), referenceSequence.begin(), referenceSequence.end());
		} else if (isIndexPattern(referenceSequence, { virt, occ, virt, occ })
				   || isIndexPattern(referenceSequence, { virt, virt, virt, occ })) {
			// H[aibj] -> J[abij] (note: b and i switched)
			// H[abci] -> J[acbi] (note: b and c switched)
			printName = "J";
			// Indices have to be reordered
			targetIndices.push_back(referenceSequence[0]);
			targetIndices.push_back(referenceSequence[2]);
			targetIndices.push_back(referenceSequence[1]);
			targetIndices.push_back(referenceSequence[3]);
		} else {
			std::cerr << tensor << std::endl;
			throw std::runtime_error(std::string("Unexpected index pattern for H: ")
									 + getIndexPatternString(referenceSequence));
		}

		writeTensorName(printName);
		writeIndexSequence(targetIndices);

		return true;
	}

	return false;
}

std::string ITFExporter::getSpinString(const std::vector< std::reference_wrapper< const ct::Index > > &indices) {
	std::string string;
	bool onlyNone = true;

	for (const ct::Index &currentIndex : indices) {
		switch (currentIndex.getSpin()) {
			case ct::Index::Spin::Alpha:
				string += "a";
				onlyNone = false;
				break;
			case ct::Index::Spin::Beta:
				string += "b";
				onlyNone = false;
				break;
			case ct::Index::Spin::None:
				string += "n";
				break;
			case ct::Index::Spin::Both:
				throw std::runtime_error(
					"ITFExporter: Encountered index with spin \"Both\" - this is not expected at this point");
				break;
		}
	}

	// If all spins have type "None", we return an empty String
	return onlyNone ? "" : string;
}

char ITFExporter::getIndexName(const ct::Index &index) const {
	char baseChar;
	unsigned int maxID;

	if (index.getSpace() == m_resolver.resolve("occupied")) {
		// i-o
		baseChar = 'i';
		maxID    = 'o' - baseChar;
	} else if (index.getSpace() == m_resolver.resolve("virtual")) {
		// a-h
		baseChar = 'a';
		maxID    = 'h' - baseChar;
	} else if (m_resolver.contains("densityfitting") && index.getSpace() == m_resolver.resolve("densityfitting")) {
		// F-K
		baseChar = 'F';
		maxID    = 'K' - baseChar;
	} else {
		throw std::runtime_error(std::string("ITFExporter: Unsupported index space: \"")
								 + m_resolver.getMeta(index.getSpace()).getName() + "\"");
	}

	if (index.getID() > maxID) {
		throw std::runtime_error(std::string("ITFExporter: Index ID overflow (") + std::to_string(index.getID())
								 + ") for space \"" + m_resolver.getMeta(index.getSpace()).getName() + "\"");
	}

	return baseChar + index.getID();
}

std::string ITFExporter::getIndexPatternString(const ct::Tensor &tensor) const {
	return getIndexPatternString(tensor.getIndices());
}

std::string ITFExporter::getIndexPatternString(const std::vector< ct::Index > &indices) const {
	std::string pattern;

	std::size_t size = indices.size();
	for (std::size_t i = 0; i < size; ++i) {
		pattern += m_resolver.getMeta(indices[i].getSpace()).getName();

		if (i + 1 < size) {
			pattern += ", ";
		}
	}

	return pattern;
}

void ITFExporter::writeTensorName(const std::string_view &name) {
	bool firstChar = true;
	for (std::size_t i = 0; i < name.size(); ++i) {
		char c = name[i];

		if (firstChar && !std::isalpha(c)) {
			throw std::runtime_error("In ITF a Tensor name must start with one of [a-zA-Z]");
		}
		firstChar = false;

		if (std::isalnum(c)) {
			// Alphanumeric characters are no problem
			*m_sink << c;
			continue;
		}

		// All other characters have to be mapped to alphanumeric ones
		switch (c) {
			case '_':
				c = '0';
				break;
			case '-':
				c = '1';
				break;
			case '\'': {
				// Apostrophes are used if two tensors would otherwise end up having the same name.
				// Since they are not valid in tensor names, replace the amount of apostrophes (plus one) by
				// a simple number preceded by "v". E.g. T' -> Tv2
				std::size_t tensorVariant = 2;
				while (i + 1 < name.size() && name[i + 1] == '\'') {
					tensorVariant++;
					i++;
				}

				std::string strNum = std::to_string(tensorVariant);
				*m_sink << "v";
				if (strNum.size() > 1) {
					// This requires more than one character -> write all but the last one to the stream here already
					*m_sink << std::string_view(strNum).substr(0, strNum.size() - 2);
				}
				c = strNum[strNum.size() - 1];
				break;
			}
			default:
				throw std::runtime_error(std::string("ITFExporter: Encountered unexpected character '") + c
										 + "' in tensor name");
		}

		*m_sink << c;
	}
}

void ITFExporter::writeIndexSequence(const std::vector< ct::Index > &indices) {
	std::vector< std::reference_wrapper< const ct::Index > > refIndices(indices.begin(), indices.end());

	writeIndexSequence(refIndices);
}

void ITFExporter::writeIndexSequence(const std::vector< std::reference_wrapper< const ct::Index > > &indices) {
	*m_sink << "[";

	for (const ct::Index &currentIndex : indices) {
		*m_sink << getIndexName(currentIndex);
	}
	*m_sink << "]";
}


}; // namespace Contractor::Formatting
