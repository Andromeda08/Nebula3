#pragma once

#include <array>
#include <expected>
#include <string>
#include <unordered_set>

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

namespace RHI
{
    class Buffer;
    class CommandList;
    class Descriptor;
    class Image;
    class VulkanRHI;
}

struct TextureManagerCreateInfo
{
    SPtr<RHI::VulkanRHI> rhi;
};

class TextureManager
{
    struct TextureLoadInfo
    {
        SPtr<RHI::Buffer> stagingBuffer;
        SPtr<RHI::Image>  textureImage;
        uint32_t          slot;
    };
public:
    static constexpr uint32_t sMaxTextureCount  = 100;
    static constexpr uint32_t sMissingTextureId = 0;

    nbl_DISABLE_COPY(TextureManager);
    nbl_CTOR(TextureManager);

    ~TextureManager() = default;

    RHI::Image* getTexture(uint32_t slot) const noexcept;

    /**
     * Loads a Texture from the specified file into a given slot.
     * [Note!] The texture will be loaded immediately for now.
     * @returns If successfully loaded true, otherwise an error message.
     */
    std::expected<bool, std::string> loadTexture(const std::string& textureFile, uint32_t slot) noexcept;

    void update(const RHI::CommandList* commandList) const;

private:
    // Blocking texture load.
    void loadImmediately(const TextureLoadInfo& textureLoadInfo) noexcept;

    // Update slot validity in metadata, mark as dirty if needed.
    void setSlot(uint32_t slot, bool hasTexture) noexcept;

    // If dirty, update the metadata texture.
    void updateMetaTexture(const RHI::CommandList* commandList) const;

    /**
     * For all textures whose loading was deferred:
     * - Set image in mTextures
     * - Write descriptor for new texture
     * - Update slot metadata
     * - Update metadata texture via updateMetaTexture()
     */
   //  void uploadQueuedTextures(const RHI::CommandList* commandList);

    // Clears the done tasks from the upload queue. (should be called from update())
    // void clearUploadQueue();

    // =====================================
    // Constructor functions
    // =====================================
    void createMetadataResources();
    void createDescriptor();
    void writeInitialDescriptors() const;

    // =====================================
    // Utility functions
    // =====================================
    [[nodiscard]] static uint64_t getTextureSize(const int w, const int h, const int c = 4) noexcept
    {
        return static_cast<uint64_t>(w) * static_cast<uint64_t>(h) * c;
    }

    static constexpr auto sMissingTextureName = "missingTexture.png";

    // Textures
    std::array<SPtr<RHI::Image>, 100>   mTextures;

    // Deferred loading
    std::vector<TextureLoadInfo>        mQueuedLoads;
    std::unordered_set<uint32_t>        mLoadInfoDeletionQueue; // By slot ID

    // Meta Texture
    bool                                mMetaIsDirty = false;
    SPtr<RHI::Image>                    mMetaTexture;
    SPtr<RHI::Buffer>                   mMetaStaging;
    std::array<int32_t, 100>            mMetaData {};

    SPtr<RHI::Descriptor>               mDescriptor;
    SPtr<RHI::VulkanRHI>                mRHI;
};
