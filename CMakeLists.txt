cmake_minimum_required(VERSION 3.10)

# Central version management
project(HIDTester VERSION 1.4.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- PATH CONFIGURATIONS ---
# Define where ImGui is located
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui")

# Tell CMake where to look for SDL3 on Windows
if(WIN32)
    # NOTE: Set this to the folder where you extracted the SDL3-devel-VC package!
    set(SDL3_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL3" CACHE PATH "Base path for SDL3")
    
    # Add it to the prefix path so find_package can locate the cmake config files
    list(APPEND CMAKE_PREFIX_PATH "${SDL3_BASE_PATH}")
endif()

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# --- FIND PACKAGES ---
# Find SDL3 globally for ALL operating systems
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

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
    "${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp"
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

# Silence MSVC warnings about deprecated C runtime functions that are used
# internally by third-party libraries (ImGui, SDL3). Scoped to this target only.
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# --- 3. CROSS-PLATFORM LINKING & POST-BUILD ---
if(WIN32)
    # Windows specific linking
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 opengl32)
    
    # Automatically copy the SDL3.dll into the build output folder next to the .exe
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_BASE_PATH}/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
elseif(APPLE)
    # macOS specific linking frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    # Standard Linux linking
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 GL)
endif()

# --- OPTIONAL: SDL3 smoke/unit tests (require SDL3) ---
# Built automatically when BUILD_TESTING is ON (the CTest default).
if(BUILD_TESTING)
    add_executable(HIDTesterTests
        tests/test_main.cpp
    )

    target_include_directories(HIDTesterTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(HIDTesterTests PRIVATE SDL3::SDL3)

    add_test(NAME HIDTesterTests COMMAND HIDTesterTests)
    set_tests_properties(HIDTesterTests PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
    )
endif()

# --- OPTIONAL: Deadzone unit tests (no SDL required) ---
# Build with: cmake -DBUILD_TESTS=ON ..
option(BUILD_TESTS "Build the deadzone unit tests (no SDL required)" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
