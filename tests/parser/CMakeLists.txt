include(GoogleTest)

add_executable(parser_test
	AbstractParserTest.cpp
	GeCCoExportParserTest.cpp
)

target_link_libraries(parser_test
	gtest_main
	${MAIN_EXECUTABLE_NAME}::parser
)

gtest_discover_tests(parser_test)
