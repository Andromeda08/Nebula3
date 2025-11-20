cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(External/json)

add_executable(Nebula3 main.cpp
    Nebula/Core/Configuration.hpp
    Nebula/VulkanRHI/RHIConfiguration.hpp
    Nebula/Core/AppSpecification.hpp
    Nebula/Core/Types.hpp
    Nebula/Core/App.hpp
    Nebula/main.cpp
    Nebula/Core/Configuration.cpp
    Nebula/Core/Macro.hpp)

target_include_directories(Nebula3 PUBLIC
    Nebula
    External/json/include)

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME})