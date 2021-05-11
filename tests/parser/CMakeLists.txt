include(GoogleTest)

set(COMPONENT_NAME "parser")

add_executable(${COMPONENT_NAME}_test
	BufferedStreamReaderTest.cpp
	GeCCoExportParserTest.cpp
)

target_link_libraries(${COMPONENT_NAME}_test
	gtest_main
	${MAIN_EXECUTABLE_NAME}::${COMPONENT_NAME}
)

gtest_discover_tests(${COMPONENT_NAME}_test)
