
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} TEST_SOURCE_FILES)
foreach(src ${TEST_SOURCE_FILES})
	string(REGEX REPLACE ".*/\(.*\).cpp$" "\\1" TEST_TARGET_NAME ${src})
	add_executable(${TEST_TARGET_NAME} ${src})
	target_link_libraries(${TEST_TARGET_NAME} spvsdk)
	target_link_libraries(${TEST_TARGET_NAME} pthread)
endforeach(src)


include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/../Include/Catch2/single_include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../SDK"
	"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Common"
	"${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Wrapper"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Core"
)