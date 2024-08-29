# run "cmake .." in the build folder

cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(GFramework VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Make sure all dll's are copied to the executable folder
#set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# EASTL
add_subdirectory(Plugins/EASTL)
list(APPEND extra_libs EASTL)

# ASSIMP
add_subdirectory(Plugins/assimp)
list(APPEND extra_libs assimp)

# Add stb_image
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/Plugins/stb_image/")

# Add GLM
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/Plugins/glm/")

# Add ImGui
add_subdirectory(Plugins/imgui)
list(APPEND extra_libs imgui)

# Add DirectXTex
add_subdirectory(Plugins/DirectXTex)
list(APPEND extra_libs DirectXTex)

# Rendering API's libs
list(APPEND extra_libs d3d11.lib)
list(APPEND extra_libs d3dcompiler.lib)

# used to add all .cpp and .h files under source
file(GLOB_RECURSE source_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Engine/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/Engine/*.h")

# used to create filters for all files one to one with their folder structure
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${source_files})

#add_library(GFramework STATIC ${source_files})
add_executable(GFramework ${source_files})

target_link_libraries(GFramework PUBLIC ${extra_libs})

target_link_libraries(GFramework PRIVATE "d3d12.lib")
target_link_libraries(GFramework PRIVATE "dxgi.lib")

# Add WinPixEventRuntime
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/Plugins/WinPixEventRuntime/Include")
target_link_libraries(GFramework PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Plugins/WinPixEventRuntime/bin/WinPixEventRuntime.lib")

# Copy command for WinPixEventRuntime.dll
add_custom_command(TARGET GFramework POST_BUILD        # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${CMAKE_CURRENT_LIST_DIR}/Plugins/WinPixEventRuntime/bin/WinPixEventRuntime.dll"      # <--this is in-file
        $<TARGET_FILE_DIR:GFramework>)                 # <--this is out-file path


list(APPEND EXTRA_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/Engine/Source)	

target_include_directories(GFramework PUBLIC
                           ${EXTRA_INCLUDES}
                           )
						   

						   

# Stop it from creating unnecessary Project
set(CMAKE_SUPPRESS_REGENERATION true)

set_target_properties(	assimp imgui EASTL EAStdC EAAssert EAThread DirectXTex
						PROPERTIES
						FOLDER "Plugins")		  
						
						
# Set 3DTest as Startup Project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GFramework)

												

