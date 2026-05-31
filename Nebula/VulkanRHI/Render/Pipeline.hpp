#pragma once

#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>
#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Descriptor.hpp"
#include "VulkanRHI/Detail/Resource.hpp"

namespace RHI
{
    struct ShaderInfo2
    {
        std::filesystem::path   path;
        vk::ShaderStageFlagBits stage;
        const char*             entryPoint = "main";

        /**
         * From the specified shader file resolve the stage using standard
         * engine file extensions and return a ShaderInfo struct.
         */
        [[nodiscard]] static ShaderInfo2 fromFileName(const std::string& fileName)
        {
            using enum vk::ShaderStageFlagBits;
            auto stage = eAll;

            // Resolve shader stage from standard file extensions
            #pragma region
            #define nbl_StageCase(Ext, Stage) if (std::string_view(substr) == Ext) { stage = Stage; }
            for (const auto substr : std::views::split(fileName, "."))
            {
                nbl_StageCase("geom",   eGeometry)
                nbl_StageCase("tc",     eTessellationControl)
                nbl_StageCase("te",     eTessellationEvaluation)
                nbl_StageCase("frag",   eFragment)
                nbl_StageCase("comp",   eCompute)
                nbl_StageCase("mesh",   eMeshEXT)
                nbl_StageCase("task",   eTaskEXT)
                nbl_StageCase("rgen",   eRaygenKHR)
                nbl_StageCase("rchit",  eClosestHitKHR)
                nbl_StageCase("rmiss",  eMissKHR)
                nbl_StageCase("rahit",  eAnyHitKHR)
                nbl_StageCase("rint",   eIntersectionKHR)
                nbl_StageCase("rcall",  eCallableKHR)
            }
            #undef nbl_StageCase
            #pragma endregion

            exitOnAssert(stage != eAll, "Failed to resolve shader stage from fileName");

            return {
                .path       = Configuration::getShaderFilePath(fileName),
                .stage      = stage,
                .entryPoint = "main"
            };
        }
    };

    /**
     * The new Pipeline class
     * Bind commands are moved to CommandList
     * PCR stored to be read by a CommandList
     */

    enum class PipelineType2 : uint8_t
    {
        Unknown    = 0,
        Compute    = 1,
        Graphics   = 2,
        Mesh       = 3, // Identify graphics pipelines that use mesh & task shaders.
        RayTracing = 4,
    };

    [[nodiscard]] constexpr std::string toString(const PipelineType2 type)
    {
        using enum PipelineType2;
        switch (type)
        {
            case Compute:       return "Compute";
            case Graphics:      return "Graphics";
            case Mesh:          return "Mesh";
            case RayTracing:    return "RayTracing";
            default:            return "Unknown";
        }
    }

    class PipelineBase : public Resource
    {
    public:
        explicit PipelineBase(const SPtr<Device>& device, const std::string& label, const PipelineType2 type)
        : Resource(device)
        , mType(type)
        , mBindPoint(toBindPoint(type))
        {
            setLabel(label);
            spdlog::debug("Creating Pipeline: {} [type={}]", styled(label, fg(fmt::color::cyan)), toString(type));
        }

        ~PipelineBase() override
        {
            mDevice->waitIdle(); // TODO: Test if this is even required
            mDevice->getHandle().destroy(mPipeline);
            mDevice->getHandle().destroy(mPipelineLayout);

            spdlog::debug("Destroyed Pipeline: {} [type={}]", styled(mLabel, fg(fmt::color::pale_violet_red)), toString(mType));
        }

        [[nodiscard]] const vk::Pipeline& getHandle() const noexcept
        {
            return mPipeline;
        }

        [[nodiscard]] const vk::PipelineLayout& getLayout() const noexcept
        {
            return mPipelineLayout;
        }

        [[nodiscard]] PipelineType2 getType() const noexcept
        {
            return mType;
        }

        [[nodiscard]] const vk::PipelineBindPoint& getBindPoint() const noexcept
        {
            return mBindPoint;
        }

        [[nodiscard]] const std::optional<vk::PushConstantRange>& getPushConstantRange() const noexcept
        {
            return mPushConstantRange;
        }

    protected:
        [[nodiscard]] static vk::PipelineBindPoint toBindPoint(const PipelineType2 type)
        {
            using enum PipelineType2;
            switch (type)
            {
                case Compute:    return vk::PipelineBindPoint::eCompute;
                case RayTracing: return vk::PipelineBindPoint::eRayTracingKHR;
                default:         return vk::PipelineBindPoint::eGraphics;
            }
        }

        vk::Pipeline            mPipeline;
        vk::PipelineLayout      mPipelineLayout;

        PipelineType2           mType       = PipelineType2::Unknown;
        vk::PipelineBindPoint   mBindPoint  = {};

        std::optional<vk::PushConstantRange> mPushConstantRange;
    };

    /**
     * Builder for common options between all pipeline types.
     */
    struct PipelineCommon
    {
        // Debug Label
        std::string label;

        PipelineCommon& setLabel(const std::string& value)
        {
            label = value;
            return *this;
        }

        // Collection of shaders used by the pipeline.
        std::vector<ShaderInfo2> shaders;

        PipelineCommon& addShader(const ShaderInfo2& shaderInfo)
        {
            shaders.push_back(shaderInfo);
            return *this;
        }

        PipelineCommon& addShader(const std::string& fileName)
        {
            shaders.push_back(ShaderInfo2::fromFileName(fileName));
            return *this;
        }

        /**
         * Descriptor Set Layouts
         * - Bindings should start from 0 and form a valid sequence with no gaps / null descriptors.
         * - Adding descriptors at the same layout overwrites and keeps the last one.
         */
        std::map<uint32_t, Descriptor*> descriptors;

        PipelineCommon& addDescriptorLayout(const uint32_t binding, Descriptor* pDescriptor)
        {
            exitOnAssert(pDescriptor != nullptr, "The added descriptor must be valid.");
            if (descriptors.contains(binding))
            {
                spdlog::warn("Descriptor binding {} was overwritten. [{} => {}]",
                    binding, descriptors[binding]->getLabel(), pDescriptor->getLabel());
            }

            descriptors[binding] = pDescriptor;
            return *this;
        }

        /**
         * Push Constant
         * - Each pipeline can only define ONE push constant block.
         * - Must start at offset = 0 (enforced by setPushConstant())
         * - Size is resolved from template param.
         */
        std::optional<vk::PushConstantRange> pushConstantRange;

        template <class T>
        PipelineCommon& setPushConstant(const vk::ShaderStageFlags stages)
        {
            pushConstantRange = { stages, 0, sizeof(T) };
            return *this;
        }
    };

    class GraphicsPipeline2 : public PipelineBase
    {
    public:

    private:

    };
}
