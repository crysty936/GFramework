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
add_subdirectory(External/EASTL)
list(APPEND extra_libs EASTL)

# ASSIMP
add_subdirectory(External/assimp)
list(APPEND extra_libs assimp)

# Add stb_image
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/External/stb_image/")

# Add GLM
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/External/glm/")

# Add ImGui
add_subdirectory(External/imgui)
list(APPEND extra_libs imgui)

# Add DirectXTex
add_subdirectory(External/DirectXTex)
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
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/External/WinPixEventRuntime/Include")
target_link_libraries(GFramework PRIVATE "${CMAKE_CURRENT_LIST_DIR}/External/WinPixEventRuntime/bin/WinPixEventRuntime.lib")

# Copy command for WinPixEventRuntime.dll
add_custom_command(TARGET GFramework POST_BUILD        # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${CMAKE_CURRENT_LIST_DIR}/External/WinPixEventRuntime/bin/WinPixEventRuntime.dll"      # <--this is in-file
        $<TARGET_FILE_DIR:GFramework>)                 # <--this is out-file path


# Add DXC
list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/External/DXCompiler/Include")
target_link_libraries(GFramework PRIVATE "${CMAKE_CURRENT_LIST_DIR}/External/DXCompiler/Lib/dxcompiler.lib")

# Copy commands for DXC
add_custom_command(TARGET GFramework POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/External/DXCompiler/Bin/dxcompiler.dll" 
        $<TARGET_FILE_DIR:GFramework>) 
		
add_custom_command(TARGET GFramework POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/External/DXCompiler/Bin/dxil.dll" 
        $<TARGET_FILE_DIR:GFramework>) 


list(APPEND EXTRA_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/Engine/Source)	

target_include_directories(GFramework PUBLIC
                           ${EXTRA_INCLUDES}
                           )
						   

						   

# Stop it from creating unnecessary Project
set(CMAKE_SUPPRESS_REGENERATION true)

set_target_properties(	assimp imgui EASTL EAStdC EAAssert EAThread DirectXTex
						PROPERTIES
						FOLDER "External")		  
						
						
# Set 3DTest as Startup Project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GFramework)

												

