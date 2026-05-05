cmake_minimum_required(VERSION 3.10)

# Central version management
project(HIDTester VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# --- PATH CONFIGURATIONS ---
# Define where ImGui is located
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui")

# Tell CMake where to look for SDL2 on Windows
if(WIN32)
    # NOTE: Set this to the folder where you extracted the SDL2-devel-VC package!
    set(SDL2_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2" CACHE PATH "Base path for SDL2")
    
    # Add it to the prefix path so find_package can locate the cmake config files
    list(APPEND CMAKE_PREFIX_PATH "${SDL2_BASE_PATH}")
endif()

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# --- FIND PACKAGES ---
# Find SDL2 globally for ALL operating systems
find_package(SDL2 REQUIRED)

# --- DIRECTORIES & SOURCES ---
include_directories(
    "${IMGUI_PATH}"
    "${IMGUI_PATH}/backends"
    "${CMAKE_CURRENT_BINARY_DIR}" # For version.h 
)

# Include Windows Resources if applicable
if(WIN32)
    set(RESOURCE_FILES "resources.rc")
endif()

file(GLOB IMGUI_SOURCES 
    "${IMGUI_PATH}/*.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp"
)

# --- 1. CREATE TARGET ---
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp 
    ${RESOURCE_FILES}
    ${IMGUI_SOURCES}
)

# --- 2. CONFIGURE TARGET ---
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# --- 3. CROSS-PLATFORM LINKING & POST-BUILD ---
if(WIN32)
    # Windows specific linking
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main opengl32)
    
    # Automatically copy the SDL2.dll into the build output folder next to the .exe
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_BASE_PATH}/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
elseif(APPLE)
    # macOS specific linking frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    # Standard Linux linking
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 GL)
endif()