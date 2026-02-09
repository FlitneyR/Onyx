cmake_minimum_required(VERSION 4.0)

set(CMAKE_TOOLCHAIN_FILE ~/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_MANIFEST_DIR "..")

project(Onyx)

file(GLOB Onyx_Src "Onyx/*.h" "Onyx/*.cpp")
add_library(Onyx)
target_sources(Onyx PUBLIC ${Onyx_Src})
target_include_directories(Onyx PUBLIC ".." ".")
set_target_properties(Onyx PROPERTIES CXX_STANDARD 20)
target_precompile_headers(Onyx PRIVATE Onyx/types.h)

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

target_link_libraries(Onyx PRIVATE
    assimp::assimp
    glm::glm
    glslang::OSDependent glslang::glslang glslang::MachineIndependent glslang::GenericCodeGen glslang::glslang-default-resource-limits glslang::OGLCompiler glslang::SPVRemapper glslang::SPIRV glslang::HLSL
    imgui::imgui
    SDL2::SDL2
    Tracy::TracyClient
    vk-bootstrap::vk-bootstrap
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)
