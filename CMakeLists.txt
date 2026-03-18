cmake_minimum_required(VERSION 4.0)

set(CMAKE_TOOLCHAIN_FILE ~/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_MANIFEST_DIR "../..")
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

if(WIN32)
    add_compile_options(/Zc:preprocessor)
    add_compile_definitions(DEBUGBREAK=__debugbreak)
endif()

if(APPLE)
    add_compile_definitions(__FUNCTION__=__PRETTY_FUNCTION__)
endif()

# Onyx Core
project(Onyx)

file(GLOB_RECURSE Onyx_Src "Onyx/*.h" "Onyx/*.cpp")
add_library(Onyx)
target_compile_definitions(Onyx PUBLIC $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Profile>:NDEBUG TRACY_ENABLE>)
target_sources(Onyx PRIVATE ${Onyx_Src})
target_include_directories(Onyx PUBLIC "." "./Onyx")
target_precompile_headers(Onyx PUBLIC Onyx/types.h Onyx/Log.h Onyx/Asserts.h)

find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_link_libraries(Onyx PUBLIC
    glm::glm
    imgui::imgui
    Tracy::TracyClient
    GPUOpen::VulkanMemoryAllocator
    unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    fmt::fmt
)

target_link_libraries(Onyx PRIVATE
    assimp::assimp
    SDL2::SDL2
    vk-bootstrap::vk-bootstrap
    Vulkan::Headers
)

target_link_libraries(Onyx PRIVATE
    glslang::GenericCodeGen
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::HLSL
    glslang::MachineIndependent
    glslang::OGLCompiler
    glslang::OSDependent
    glslang::SPIRV
    glslang::SPVRemapper
)

# Asteroids Demo
file(GLOB_RECURSE Asteroids_Common_Headers "Asteroids/Common/*.h")
file(GLOB_RECURSE Asteroids_Common_Src "Asteroids/Common/*.cpp")
file(GLOB_RECURSE Asteroids_Game_Src "Asteroids/Game/*.h" "Asteroids/Game/*.cpp")
file(GLOB_RECURSE Asteroids_Editor_Src "Asteroids/Editor/*.h" "Asteroids/Editor/*.cpp")

add_library(Asteroids_Common)
add_executable(Asteroids_Game)
add_executable(Asteroids_Editor)

target_sources(Asteroids_Common PUBLIC ${Asteroids_Common_Headers})
target_sources(Asteroids_Common PRIVATE ${Asteroids_Common_Src})
target_sources(Asteroids_Game PRIVATE ${Asteroids_Game_Src})
target_sources(Asteroids_Editor PRIVATE ${Asteroids_Editor_Src})

target_link_libraries(Asteroids_Common PRIVATE Onyx)
target_link_libraries(Asteroids_Game PRIVATE Asteroids_Common Onyx)
target_link_libraries(Asteroids_Editor PRIVATE Asteroids_Common Onyx)
