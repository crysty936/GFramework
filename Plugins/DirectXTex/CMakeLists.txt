# If this lib does not work, most likely reason is that the shaders need to be recompiled, in which case the whole repo needs to be downloaded and recompiling can be done using its CMake build process

file(GLOB_RECURSE source_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

add_library(DirectXTex STATIC ${source_files})

target_include_directories(DirectXTex PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Shaders/Compiled")
target_include_directories(DirectXTex PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
