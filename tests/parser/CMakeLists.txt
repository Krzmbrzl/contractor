include(GoogleTest)

set(COMPONENT_NAME "parser")

add_executable(${COMPONENT_NAME}_test
	BufferedStreamReaderTest.cpp
	GeCCoExportParserTest.cpp
	SymmetryListParserTest.cpp
	IndexSpaceParserTest.cpp
	DecompositionParserTest.cpp
)

target_link_libraries(${COMPONENT_NAME}_test
	gtest_main
	gmock
	${MAIN_EXECUTABLE_NAME}::${COMPONENT_NAME}
	${MAIN_EXECUTABLE_NAME}::utils
	${MAIN_EXECUTABLE_NAME}::terms
)

gtest_discover_tests(${COMPONENT_NAME}_test)
