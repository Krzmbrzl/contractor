#include "formatting/ITFExporter.hpp"

#include "terms/Index.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

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

void ITFExporter::writeTensor(const ct::Tensor &tensor) {
	assert(m_sink != nullptr);

	// Sort indices such that the ones belonging to the biggest index space are located to the left. We assume that the
	// Tensors have been renamed in such a way that this reordering of indices does not cause change ambiguity as to
	// which Tensor is being referenced.
	std::vector< std::reference_wrapper< const ct::Index > > indices(tensor.getIndices().begin(),
																	 tensor.getIndices().end());
	std::vector< std::reference_wrapper< const ct::Index > > originalIndices = indices;
	std::stable_sort(indices.begin(), indices.end(), [this](const ct::Index &left, const ct::Index &right) {
		if (m_resolver.getMeta(left.getSpace()).getSize() != m_resolver.getMeta(right.getSpace()).getSize()) {
			return m_resolver.getMeta(left.getSpace()).getSize() > m_resolver.getMeta(right.getSpace()).getSize();
		}

		return left < right;
	});

	std::string spinString         = getSpinString(indices);
	std::string originalSpinString = getSpinString(originalIndices);

	*m_sink << tensor.getName();
	if (m_isIntermediate(tensor.getName())) {
		// Mark intermediates (STIN = "short-term-intermediate")
		*m_sink << "_STIN";
	}
	if (!spinString.empty()) {
		// Mark explicit spin cases (before and after sorting)
		*m_sink << "_" << originalSpinString << "_" << spinString;
	}

	// Print list of indices
	*m_sink << "[";

	for (const ct::Index &currentIndex : indices) {
		*m_sink << getIndexName(currentIndex);
	}
	*m_sink << "]";
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

}; // namespace Contractor::Formatting
