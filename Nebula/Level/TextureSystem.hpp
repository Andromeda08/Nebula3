#pragma once

#include <array>
#include <queue>
#include <unordered_map>

#include <stb_image.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_hash.hpp>

#include "Core/Types.hpp"
#include "glm/detail/func_common.inl"
#include "VulkanRHI/Descriptor.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class [[nodiscard]] TextureSystem
    {
        enum class Bindings : uint32_t
        {
            bTextureArray    = 0,
            bSamplerArray    = 1,
            bMetadata        = 2,
        };

        struct TextureLoadRequest
        {
            stbi_uc*           pixels;
            int32_t            width;
            int32_t            height;

            std::optional<std::string>           name         = std::nullopt;
            std::optional<vk::Format>            format       = std::nullopt;
            std::optional<vk::SamplerCreateInfo> samplerState = std::nullopt;
        };

        struct UploadRequest
        {
            std::variant<stbi_uc*, std::vector<uint8_t>> data;
            uint8_t                                      nChannels;
            uint64_t                                     sizeBytes;
            std::size_t                                  slot;

            const void* getPData() const
            {
                if (std::holds_alternative<stbi_uc*>(data))
                {
                    return std::get<stbi_uc*>(data);
                }
                if (std::holds_alternative<std::vector<uint8_t>>(data))
                {
                    return std::get<std::vector<uint8_t>>(data).data();
                }
                return nullptr;
            }

            ~UploadRequest()
            {
                if (std::holds_alternative<stbi_uc*>(data))
                {
                    stbi_image_free(std::get<stbi_uc*>(data));
                }
            }
        };

        struct TransientBuffer
        {
            SPtr<RHI::Buffer> buffer;
            uint64_t          releaseFrame;

            // Free the Buffer if the current frame has passed or equals the release frame.
            [[nodiscard]] bool tryFree(const RHI::FrameData& frameData)
            {
                if (frameData.currentFrame >= releaseFrame)
                {
                    buffer.reset();
                    return true;
                }
                return false;
            }
        };

        struct MetadataEntry
        {
            int32_t  isValid;
            uint32_t samplerIndex;
        };

    public:
        explicit TextureSystem(const SPtr<RHI::VulkanRHI>& rhi)
        : mRHI(rhi)
        {
            mAnisotropy    = mRHI->getDevice()->getDeviceExtensions().getFeatures().samplerAnisotropy;
            mMaxAnisotropy = mRHI->getDevice()->getPhysicalDevice().getProperties().limits.maxSamplerAnisotropy;

            for (uint64_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                mMetadataBuffer[i] = mRHI->createBuffer({
                    .size  = sMaxTextures * sizeof(MetadataEntry),
                    .type  = RHI::BufferType::Storage,
                    .label = fmt::format("TextureSystem-Meta-{}", i),
                });
            }

            createDefaultImmutableSampler();
            createDescriptor();

            for (std::size_t i = 0; i < sMaxTextures; i++)
            {
                mFreeTextureSlots.push(i);
            }

            // Skip default sampler
            for (std::size_t i = 0; i < sMaxSamplers; i++)
            {
                if (i == sDefaultSampler)
                {
                    continue;
                }
                mFreeSamplerSlots.push(i);
            }

            createFallbackTexture();

            writeInitialDescriptors();
        }

        ~TextureSystem()
        {
            // Clean up samplers
            for (const auto& sampler : mSamplers)
            {
                if (sampler)
                {
                    mRHI->getDevice()->getHandle().destroy(sampler);
                }
            }
        }

        uint32_t loadTextureFromMemory(const TextureLoadRequest& loadRequest)
        {
            const auto samplerSlot = createSampler(loadRequest.samplerState);
            const auto textureSlot = createImage(
                { static_cast<uint32_t>(loadRequest.width), static_cast<uint32_t>(loadRequest.height) },
                loadRequest.name, loadRequest.format);

            mSamplerAssignment[textureSlot] = samplerSlot;

            mUploadQueue.push_back({
                .data      = loadRequest.pixels,
                .nChannels = 4,
                .sizeBytes = loadRequest.width * loadRequest.height * 4 * sizeof(stbi_uc),
                .slot      = textureSlot,
            });

            return textureSlot;
        }

        void releaseTexture(uint32_t textureSlot)
        {
            // TODO: Release
        }

        void onUpdate(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
        {
            onUpdate_MaintainTransientBuffers(pCommandList, frameData);

            onUpdate_RunUploadQueue(pCommandList, frameData);

            onUpdate_WriteMetadata(pCommandList, frameData);
        }

    private:
        // Check transient buffers and remove the ones whose time has expired.
        void onUpdate_MaintainTransientBuffers([[maybe_unused]] const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
        {
            for (auto it = mTransientBuffers.begin(); it != mTransientBuffers.end();)
            {
                if (it->tryFree(frameData))
                {
                    it = mTransientBuffers.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        // Flush upload queue
        void onUpdate_RunUploadQueue(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
        {
            if (mUploadQueue.empty())
            {
                return;
            }

            uint64_t stagingSize = 0;
            for (const auto& item : mUploadQueue)
            {
                stagingSize += item.sizeBytes;
            }

            mTransientBuffers.push_back(TransientBuffer {
                .buffer = mRHI->createBuffer({
                    .size  = stagingSize,
                    .type  = RHI::BufferType::Staging,
                    .label = fmt::format("TextureSystem-Staging-Frame#{}", frameData.currentFrame),
                }),
                .releaseFrame = frameData.currentFrame + RHI::gFramesInFlight,
            });
            const auto& [buffer, releaseFrame] = mTransientBuffers.back();

            std::vector<uint32_t> stagingOffsets = { 0 };
            stagingOffsets.reserve(mUploadQueue.size());
            auto preBarrier = RHI::Barrier();

            for (const auto& item : mUploadQueue)
            {
                buffer->setData(item.getPData(), item.sizeBytes, stagingOffsets.back());
                stagingOffsets.push_back(stagingOffsets.back() + item.sizeBytes);

                preBarrier.addImageBarrier({
                   .dstUsage = RHI::ImageUsage::TransferDst,
                   .image    = mResidentTextures[item.slot],
               });
            }
            preBarrier
                .addBarrier(buffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferSrc))
                .insert(pCommandList);

            for (const auto& [idx, item] : nbl::enumerate(mUploadQueue))
            {
                pCommandList->copyBufferToImage({
                    .pSrcBuffer   = buffer.get(),
                    .pDstImage    = mResidentTextures[item.slot].get(),
                    .bufferOffset = stagingOffsets[idx],
                });

                mResidentTextures[item.slot]->generateMipmaps(pCommandList, vk::Filter::eLinear);
            }

            auto postBarrier = RHI::Barrier();
            for (const auto& item : mUploadQueue)
            {
                postBarrier.addImageBarrier({
                   .dstUsage = RHI::ImageUsage::ShaderReadOnly,
                   .image    = mResidentTextures[item.slot],
               });

                mTextureValidity[item.slot] = true;
            }
            postBarrier.insert(pCommandList);

            // Clear upload queue, this frees CPU-side texture data.
            mUploadQueue = {};
        }

        // Update metadata buffer
        void onUpdate_WriteMetadata(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
        {
            // Rewrite the whole thing
            std::array<MetadataEntry, sMaxTextures> meta;
            for (size_t i = 0; i < sMaxTextures; i++)
            {
                meta[i] = MetadataEntry {
                    .isValid      = static_cast<int32_t>(mTextureValidity[i]),
                    .samplerIndex = static_cast<uint32_t>(mTextureValidity[i] ? sDefaultSampler : mSamplerAssignment[i]),
                };
            }

            mTransientBuffers.push_back(TransientBuffer {
                .buffer = mRHI->createBuffer({
                    .size  = sMaxTextures * sizeof(MetadataEntry),
                    .type  = RHI::BufferType::Staging,
                    .label = fmt::format("TextureSystem-Staging-Meta-Frame#{}", frameData.currentFrame),
                }),
                .releaseFrame = frameData.currentFrame + RHI::gFramesInFlight,
            });
            const auto& [buffer, releaseFrame] = mTransientBuffers.back();

            buffer->setData(meta.data(), sMaxTextures * sizeof(MetadataEntry), 0);

            pCommandList->copyBuffer({
                .src = buffer.get(),
                .dst = mMetadataBuffer[frameData.currentFrame].get(),
            });
        }

        void createDefaultImmutableSampler()
        {
            if (const auto slot = createSampler(); slot != sDefaultSampler)
            {
                exitWithError("[TextureSystem] Unexpected slot encountered when initializing the default sampler, expected {}, got {}.",
                    sDefaultSampler, slot);
            }
        }

        void createDescriptor()
        {
            using enum vk::DescriptorType;
            const auto stageFlags = getStageFlags();

            mDescriptor = mRHI->createDescriptor({
                .bindings  = {
                    vk::DescriptorSetLayoutBinding()
                        .setBinding(static_cast<uint32_t>(Bindings::bTextureArray))
                        .setDescriptorCount(sMaxTextures)
                        .setDescriptorType(eSampledImage)
                        .setStageFlags(stageFlags),
                    vk::DescriptorSetLayoutBinding()
                        .setBinding(static_cast<uint32_t>(Bindings::bSamplerArray))
                        .setDescriptorCount(sMaxSamplers)
                        .setDescriptorType(eSampler)
                        .setStageFlags(stageFlags),
                    vk::DescriptorSetLayoutBinding()
                        .setBinding(static_cast<uint32_t>(Bindings::bMetadata))
                        .setDescriptorCount(1)
                        .setDescriptorType(eStorageBuffer)
                        .setStageFlags(stageFlags),
                },
                .setCount  = RHI::gFramesInFlight,
                .debugName = "TextureSystem",
            });
        }

        void writeInitialDescriptors() const
        {
            std::array<vk::DescriptorImageInfo, sMaxTextures> textureInfos;
            for (size_t i = 0; i < sMaxTextures; i++)
            {
                textureInfos[i] = vk::DescriptorImageInfo()
                    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setImageView(mResidentTextures[sFallbackTexture]->getImageView())
                    .setSampler(VK_NULL_HANDLE);
            }

            std::array<vk::DescriptorImageInfo, sMaxTextures> samplerInfos;
            for (size_t i = 0; i < sMaxSamplers; i++)
            {
                samplerInfos[i] = vk::DescriptorImageInfo()
                    .setImageLayout(vk::ImageLayout::eUndefined)
                    .setImageView(VK_NULL_HANDLE)
                    .setSampler(mSamplers[sDefaultSampler]);
            }

            for (uint64_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                const auto write = RHI::DescriptorWrite()
                    .writeSampledImages(static_cast<uint32_t>(Bindings::bTextureArray), textureInfos)
                    .writeSamplers(static_cast<uint32_t>(Bindings::bSamplerArray), samplerInfos)
                    .writeStorageBuffer(static_cast<uint32_t>(Bindings::bMetadata), mMetadataBuffer[i]);
                mDescriptor->write(i, write);
            }
        }

        /**
         * Create a magenta/black checkered texture in memory and queue the upload to the GPU.
         */
        void createFallbackTexture()
        {
            static constexpr uint32_t sDim = 32;
            static constexpr uint32_t sChannels = 4;

            // Generate checkerboard
            std::vector<uint8_t> data;
            data.reserve(sDim * sDim * sChannels); // 32x32, RGBA
            for (size_t y = 0; y < sDim; y++)
            {
                const auto blockY = static_cast<int32_t>(y / 4);
                for (size_t x = 0; x < sDim; x++)
                {
                    const auto blockX = static_cast<int32_t>(x / 4);
                    std::array<uint8_t, sChannels> color = { 0, 0, 0, 255 };
                    if (((blockX + blockY) % 2) == 0)
                    {
                        color[0] = color[2] = 255;
                    }
                    data.append_range(color);
                }
            }

            // Create image
            const auto slot = createImage({ sDim, sDim }, "FallbackTexture");
            if (slot != sFallbackTexture)
            {
                exitWithError("[TextureSystem] Unexpected slot encountered when initializing the fallback texture, expected {}, got {}.",
                    sFallbackTexture, slot);
            }

            // Make upload request
            mUploadQueue.push_back({
                .data      = std::move(data),
                .nChannels = sChannels,
                .sizeBytes = sDim * sDim * sChannels * sizeof(uint8_t),
                .slot      = slot,
            });
        }

        /**
         * Create an Image in the next available slot and return its index.
         * TODO: Writes it into descriptors at the acquired slot.
         */
        uint32_t createImage(
            const vk::Extent2D                extent,
            const std::optional<std::string>& name = std::nullopt,
            const std::optional<vk::Format>&  format = std::nullopt)
        {
            const auto slot = getNextImageSlot();

            using enum vk::ImageUsageFlagBits;
            mResidentTextures[slot] = mRHI->createImage({
                .extent        = extent,
                .format        = format.value_or(sDefaultFormat),
                .usageFlags    = eSampled | eTransferDst | eTransferSrc | eStorage,
                .samples       = vk::SampleCountFlagBits::e1,
                .createSampler = false,
                .aliased       = false,
                .mipmapping    = true,
                .cubeMap       = false,
                .debugName     = fmt::format("[TextureSystem | Slot = {}] {}", slot, name.value_or(fmt::format("Texture #{}", slot))),
            });

            // TODO: Write descriptor: texture

            return slot;
        }

        /**
         * Create a Sampler if the state is unique. Returns the index of the newly created sampler
         * or that of the existing one. If no state is specified the glTF defaults are assumed.
         * TODO: Writes it into descriptors at the acquired slot.
         * (Linear mipmap and filtering and Repeat UVW address modes, Opaque Black border color)
         */
        uint32_t createSampler(const std::optional<vk::SamplerCreateInfo>& createInfo = std::nullopt)
        {
            const auto stateInfo = createInfo.value_or(getDefaultSamplerState());
            if (mStateToSamplerIndex.contains(stateInfo))
            {
                return mStateToSamplerIndex.at(stateInfo);
            }

            const std::size_t slot = getNextSamplerSlot();
            mSamplers[slot] = mRHI->getDevice()->getHandle().createSampler(stateInfo);
            mStateToSamplerIndex.insert({ stateInfo, slot });

            // TODO: Write descriptor: sampelr array

            return slot;
        }

        vk::SamplerCreateInfo getDefaultSamplerState() const
        {
            return vk::SamplerCreateInfo()
                .setAnisotropyEnable(mAnisotropy)
                .setMaxAnisotropy(mMaxAnisotropy)
                .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(false)
                .setCompareEnable(false)
                .setCompareOp(vk::CompareOp::eAlways)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear);
        }

        std::size_t getNextSamplerSlot()
        {
            if (mFreeSamplerSlots.empty())
            {
                exitWithError("There are no free Sampler slots (max: {})", sMaxSamplers);
            }

            const auto nextSlot = mFreeSamplerSlots.front();
            mFreeSamplerSlots.pop();
            return nextSlot;
        }

        std::size_t getNextImageSlot()
        {
            if (mFreeTextureSlots.empty())
            {
                exitWithError("There are no free Texture slots (max: {})", sMaxTextures);
            }

            const auto nextSlot = mFreeTextureSlots.front();
            mFreeTextureSlots.pop();
            return nextSlot;
        }

        vk::ShaderStageFlags getStageFlags() const
        {
            using enum vk::ShaderStageFlagBits;
            vk::ShaderStageFlags flags = eFragment | eCompute;
            if (mRHI->getFeatures().rayTracing)
            {
                flags |= eRaygenKHR | eClosestHitKHR | eMissKHR | eCallableKHR;
            }
            if (mRHI->getFeatures().meshShaders)
            {
                flags |= eMeshEXT | eTaskEXT;
            }
            return flags;
        }

        static constexpr std::size_t sDefaultSampler  = 0;
        static constexpr std::size_t sFallbackTexture = 0;
        static constexpr std::size_t sMaxSamplers     = 64;
        static constexpr std::size_t sMaxTextures     = 1024;
        static constexpr auto        sDefaultFormat   = vk::Format::eR8G8B8A8Srgb;

        SPtr<RHI::VulkanRHI> mRHI;

        // Textures
        std::array<SPtr<RHI::Image>, sMaxTextures>  mResidentTextures;
        std::array<std::size_t,      sMaxTextures>  mSamplerAssignment;
        std::array<bool,             sMaxTextures>  mTextureValidity;
        std::queue<std::size_t>                     mFreeTextureSlots;

        // Samplers
        bool                                        mAnisotropy = false;
        float                                       mMaxAnisotropy = 0.0f;
        std::array<vk::Sampler,      sMaxSamplers>  mSamplers;
        std::queue<std::size_t>                     mFreeSamplerSlots;

        std::unordered_map<vk::SamplerCreateInfo, std::size_t> mStateToSamplerIndex;

        // Metadata
        std::vector<TransientBuffer>                mTransientBuffers;
        PerFrameArray<SPtr<RHI::Buffer>>            mMetadataBuffer;

        // Upload
        std::vector<UploadRequest>                  mUploadQueue;

        // Descriptor
        SPtr<RHI::Descriptor>                       mDescriptor;
    };
}
