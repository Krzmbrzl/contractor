include(GoogleTest)

set(COMPONENT_NAME "terms")

add_executable(${COMPONENT_NAME}_test
	IndexSpaceTest.cpp
	IndexTest.cpp
	TensorTest.cpp
	TermTest.cpp
	GeneralTermTest.cpp
	BinaryTermTest.cpp
	IndexPermutationTest.cpp
)

target_link_libraries(${COMPONENT_NAME}_test
	gtest_main
	${MAIN_EXECUTABLE_NAME}::${COMPONENT_NAME}
)

gtest_discover_tests(${COMPONENT_NAME}_test)
