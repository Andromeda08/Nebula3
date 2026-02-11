add_executable(spv_cross_msl_test spv_cross_msl.test.cpp)
target_link_libraries(spv_cross_msl_test PRIVATE
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-msl
    spdlog::spdlog
)
target_include_directories(spv_cross_msl_test PRIVATE
    ../External/spirv-cross/include
    ../External/spdlog/include
)