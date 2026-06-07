#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "Game/FontFace.hpp"


namespace nbl
{
    struct FontSource
    {
        std::filesystem::path   file;
        std::string             name;
    };

    struct FontKey
    {
        int32_t     size;
        std::string name;

        auto operator<=>(const FontKey&) const = default;
    };

    class FontLibrary
    {
    public:
        explicit FontLibrary(TextureManager* pTextureManager)
        : mTextureManager(pTextureManager)
        {
        }

        void registerSource(const FontSource& source)
        {
            if (mFontSources.contains(source.name))
            {
                return;
            }

            mFontSources.insert_or_assign(source.name, source);
        }

        [[nodiscard]] FontFace* getFont(const std::string& name, const int32_t size)
        {
            const FontKey key = { size, name };
            if (!mFonts.contains(key))
            {
                mFonts.insert_or_assign(key, makeUnique<FontFace>(mTextureManager, mFontSources[name].file, static_cast<float>(size)));
            }

            return mFonts[key].get();
        }

    private:
        TextureManager*                   mTextureManager;
        std::map<std::string, FontSource> mFontSources;
        std::map<FontKey, UPtr<FontFace>> mFonts;
    };

    class IElement
    {
    public:
        [[nodiscard]] virtual       glm::vec2   getExtent()   = 0;
        [[nodiscard]] virtual const UIGeometry& getGeometry() = 0;

        [[nodiscard]] virtual uint32_t getTextureIndex() const = 0;

        virtual ~IElement() = default;
    };

    class TextElement : public IElement
    {
    public:
        TextElement(const std::string& text, const glm::vec3& color, FontFace* pFontFace)
        : IElement()
        , mText(text)
        , mColor(glm::vec4(color, 1.0f))
        , mFont(pFontFace)
        {
        }

        ~TextElement() override = default;

        [[nodiscard]] glm::vec2 getExtent() override
        {
            const TextBounds b = mFont->measureTextBounds(mText);
            return { b.width, b.ascent + b.descent };
        }

        [[nodiscard]] const UIGeometry& getGeometry() override
        {
            if (mGeometry.vertices.empty())
            {
                mGeometry = mFont->buildTextGeometry(mText, mPosition);
            }
            return mGeometry;
        }

        [[nodiscard]] uint32_t getTextureIndex() const override
        {
            return mFont->getTextureIndex();
        }

        std::string mText;
        glm::vec4   mColor;
        FontFace*   mFont;

        glm::vec2   mPosition = glm::vec2(0.0f);
        UIGeometry  mGeometry = {};
    };

    struct ElemGeometryInfo
    {
        uint32_t firstIndex  = 0;
        uint32_t indexCount  = 0;
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
    };

    enum class Location
    {
        Center
    };

    struct InterfaceSize
    {
        glm::vec2 size;
        glm::vec2 padding;

        glm::vec2 center     {};   // Real center
        glm::vec2 origin     {};   // Origin accounting for padding
        glm::vec2 extent     {};   // Extent accounting for padding
        glm::mat4 projection {};   // Ortho projection

        explicit InterfaceSize(const glm::vec2& _size, const glm::vec2& _padding = glm::vec2(0.0f))
        : size(_size)
        , padding(_padding)
        {
            center     = size / 2.0f;
            origin     = padding;
            extent     = size - padding;
            projection = glm::orthoLH_ZO(0.0f, size.x, 0.0f, size.y, 0.0f, 1.0f);
        }

        [[nodiscard]] glm::vec2 getPosition(const Location location, IElement* element) const
        {
            exitOnAssert(element, "Invalid element");

            const auto elemSize = element->getExtent();

            using enum Location;
            switch (location)
            {
                case Center:
                default: {
                    return center - elemSize;
                }
            }
        }

        [[nodiscard]] vk::Rect2D getRenderArea() const noexcept
        {
            return vk::Rect2D()
                .setOffset({ 0, 0 })
                .setExtent({ static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y) });
        }
    };

    struct InterfaceParams
    {
        glm::vec2               size;
        glm::vec2               padding;
        SPtr<RHI::VulkanRHI>    rhi;
        TextureManager*         pTextureManager;
        std::vector<FontSource> fonts;
        bool                    msaa;
    };

    class Interface
    {
        struct PushConstant
        {
            glm::mat4 proj;
            glm::vec4 color;
            int32_t   textureIndex = -1;
            int32_t   isText = 0;
        };
    public:
        explicit Interface(const InterfaceParams& params);

        void render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData);

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t frame)
        {
            return mResolvedResult[frame];
        }

    private:
        SPtr<RHI::VulkanRHI>                    mRHI;
        TextureManager*                         mTextureManager;
        UPtr<FontLibrary>                       mFontLibrary;

        InterfaceSize                           mSize;

        std::vector<UPtr<IElement>>             mElements;
        std::vector<ElemGeometryInfo>           mGeometryInfo;

        SPtr<RHI::Buffer>                       mVertexBuffer;
        SPtr<RHI::Buffer>                       mIndexBuffer;

        SPtr<RHI::Pipeline>                     mPipeline;
        PerFrameArray<SPtr<RHI::RenderPass>>    mRenderPass;
        PerFrameArray<SPtr<RHI::Image>>         mResult;
        PerFrameArray<SPtr<RHI::Image>>         mResolvedResult;
    };
}
