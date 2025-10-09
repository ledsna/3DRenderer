cmake_minimum_required(VERSION 3.10)
project(3DRenderer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

option(USE_SYSTEM_SFML "Use system-installed SFML instead of submodule" OFF)
option(USE_SYSTEM_EIGEN "Use system-installed Eigen instead of submodule" OFF)

if(USE_SYSTEM_EIGEN)
    find_package(Eigen3 3.4 REQUIRED NO_MODULE)
    message(STATUS "Using system Eigen")
else()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/eigen/CMakeLists.txt")
        message(FATAL_ERROR "Eigen submodule not found. Run: git submodule update --init --recursive")
    endif()
    add_subdirectory(dependencies/eigen EXCLUDE_FROM_ALL)
    message(STATUS "Using Eigen submodule")
endif()

if(USE_SYSTEM_SFML)
    find_package(SFML 3 COMPONENTS Graphics Window System)
    if(NOT SFML_FOUND)
        find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)
    endif()
    message(STATUS "Using system SFML")
else()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SFML/CMakeLists.txt")
        message(FATAL_ERROR "SFML submodule not found. Run: git submodule update --init --recursive")
    endif()
    
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    
    add_subdirectory(dependencies/SFML EXCLUDE_FROM_ALL)
    message(STATUS "Using SFML submodule")
endif()

set(SOURCE_FILES
    source/main.cpp
    source/engine/Application.cpp
    source/engine/Camera.cpp
    source/engine/Renderer.cpp
    source/graphics/RenderPipeline.cpp
    source/graphics/Screen.cpp
    source/graphics/SelectionHighlight.cpp
    source/graphics/Shader.cpp
    source/scene/Object.cpp
    source/scene/Transform.cpp
    source/assets/Mesh.cpp
    source/assets/ObjLoader.cpp
    source/input/OverlayUI.cpp
    source/input/TransformGizmo.cpp
    source/input/UIComponents.cpp
    source/utils/FileDialog.cpp
)

set(HEADER_FILES
    source/engine/Application.h
    source/engine/Camera.h
    source/engine/Renderer.h
    source/graphics/Color.h
    source/graphics/DrawUtils.h
    source/graphics/RenderContext.h
    source/graphics/RenderPipeline.h
    source/graphics/Screen.h
    source/graphics/SelectionHighlight.h
    source/graphics/Shader.h
    source/graphics/ShadowMap.h
    source/graphics/Viewport.h
    source/scene/Object.h
    source/scene/Transform.h
    source/scene/World.h
    source/scene/Material.h
    source/assets/Mesh.h
    source/assets/ObjLoader.h
    source/input/OverlayUI.h
    source/input/TransformGizmo.h
    source/input/UIComponents.h
    source/utils/BoundingBox.h
    source/utils/BoundingBoxUtils.h
    source/utils/Dimensions.h
    source/utils/EdgeFunctionUtils.h
    source/utils/FileDialog.h
    source/utils/Frustum.h
    source/utils/Logger.h
    source/utils/MathUtils.h
    source/utils/Ray.h
    source/utils/Result.h
    source/utils/TriangleClipper.h
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Eigen3::Eigen
    sfml-graphics
    sfml-window
    sfml-system
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE
        OUTPUT_NAME "3d-renderer"
    )
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "3d-renderer"
    )
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        OUTPUT_NAME "3d-renderer"
    )
    
    if(NOT USE_SYSTEM_SFML)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${SOURCE_FILES} ${HEADER_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format on source files"
    )
else()
    message(STATUS "clang-format not found")
endif()

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE}
        -checks=*,-fuchsia-*,-google-*,-zircon-*,-abseil-*,-modernize-use-trailing-return-type
    )
else()
    message(STATUS "clang-tidy not found")
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "3D Renderer Configuration")
message(STATUS "========================================")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:        ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System Eigen:    ${USE_SYSTEM_EIGEN}")
message(STATUS "System SFML:     ${USE_SYSTEM_SFML}")
message(STATUS "========================================")
message(STATUS "")