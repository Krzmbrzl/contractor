include(GoogleTest)

set(COMPONENT_NAME "utils")

add_executable(${COMPONENT_NAME}_test
	IterableTest.cpp
	IndexSpaceResolverTest.cpp
	PairingGeneratorTest.cpp
)

target_include_directories(${COMPONENT_NAME}_test
	PRIVATE "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(${COMPONENT_NAME}_test
	gtest_main
	${MAIN_EXECUTABLE_NAME}::${COMPONENT_NAME}
	${MAIN_EXECUTABLE_NAME}::utils
)

gtest_discover_tests(${COMPONENT_NAME}_test)
