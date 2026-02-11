#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include <spirv_msl.hpp>
#include <spdlog/spdlog.h>

class msl
{
public:
    explicit msl(const std::string_view filePath)
    : mFilePath(filePath)
    {
        if (mFilePath.extension() != ".spv")
        {
            spdlog::error("Expected SPIRV file to have an extension of .spv");
            std::exit(EXIT_FAILURE);
        }

        const auto spv = readSPIRV();
        mCompiler = std::make_unique<spirv_cross::CompilerMSL>(spv);
    }

    void writeMSL() const noexcept
    {
        auto outputPath = mFilePath;
        outputPath.replace_extension(".metal");

        std::ofstream os(outputPath);
        os << mCompiler->compile();
        os.close();
    }

private:
    std::vector<uint32_t> readSPIRV() const noexcept
    {
        std::ifstream file(mFilePath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::println("Failed to read file: {}", mFilePath.string());
            std::exit(EXIT_FAILURE);
        }

        const std::streamsize length = file.tellg();
        file.seekg(0);

        std::vector<uint32_t> buffer;
        buffer.resize(length / sizeof(uint32_t));

        file.read(reinterpret_cast<char*>(buffer.data()), length);

        file.close();
        return buffer;
    }

    std::filesystem::path                     mFilePath;
    std::unique_ptr<spirv_cross::CompilerMSL> mCompiler;
};

int main()
{
    const auto shader = msl("Lighting.frag.spv");

    shader.writeMSL();

    return 0;
}
