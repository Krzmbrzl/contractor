include(GoogleTest)

set(COMPONENT_NAME "processor")

add_executable(${COMPONENT_NAME}_test
	FactorizerTest.cpp
	SpinIntegratorTest.cpp
	SymmetrizerTest.cpp
	SpinSummationTest.cpp
	SimplifierTest.cpp
)

target_link_libraries(${COMPONENT_NAME}_test
	gmock
	gtest_main
	${MAIN_EXECUTABLE_NAME}::${COMPONENT_NAME}
	${MAIN_EXECUTABLE_NAME}::terms
)

gtest_discover_tests(${COMPONENT_NAME}_test)
