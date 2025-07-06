file(GLOB_RECURSE source_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

add_library(HosekSky STATIC ${source_files})

target_include_directories(HosekSky PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

set_target_properties(HosekSky PROPERTIES LINKER_LANGUAGE CXX)