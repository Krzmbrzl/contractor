set(LIB_ALIAS "terms")
set(LIB_NAME "${MAIN_EXECUTABLE_NAME}_${LIB_ALIAS}")

add_library(${LIB_NAME} STATIC
	Tensor.cpp
	Term.cpp
	GeneralTerm.cpp
	BinaryTerm.cpp
	IndexSubstitution.cpp
	IndexSpaceMeta.cpp
	TensorDecomposition.cpp
	PermutationGroup.cpp
	TensorSubstitution.cpp
	TensorRename.cpp
)

add_library(${MAIN_EXECUTABLE_NAME}::${LIB_ALIAS} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
	# Required for the multiprecision integer type
	PUBLIC Boost::boost
)

target_include_directories(${LIB_NAME}
	PUBLIC "${CMAKE_SOURCE_DIR}/include"
)
