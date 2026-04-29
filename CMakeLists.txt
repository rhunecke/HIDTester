cmake_minimum_required(VERSION 3.10)

# Central version management
project(HIDTester VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# --- CROSS-PLATFORM LIBRARY SETUP ---
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui")

if(WIN32)
    # Windows: Use local thirdparty folder
    set(SDL2_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2")
    find_package(SDL2 REQUIRED CONFIG PATHS "${SDL2_BASE_PATH}/cmake")
    include_directories("${SDL2_BASE_PATH}/include")
else()
    # Linux: Use system packages
    find_package(SDL2 REQUIRED)

endif()

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

add_executable(${PROJECT_NAME} WIN32
    src/main.cpp 
    ${RESOURCE_FILES}
    ${IMGUI_SOURCES}
)

# --- CROSS-PLATFORM LINKING & POST-BUILD ---
if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main opengl32)
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_BASE_PATH}/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    # --- Direct use of SDL2::SDL2 Target ---
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 GL)
endif()