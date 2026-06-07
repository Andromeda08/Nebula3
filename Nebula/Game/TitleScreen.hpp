#pragma once

#include <optional>
#include <ranges>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "FontFace.hpp"
#include "UIGeometry.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

enum class Anchor
{
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    Center,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

struct UIScreen
{
    glm::vec2 size;

    // Global padding in pixels
    glm::vec2 padding = { 64.0f, 64.0f };

    // Orthographic projection matrix
    glm::mat4 proj;

    explicit UIScreen(const glm::vec2& _size)
    : size(_size)
    {
        proj = glm::orthoLH_ZO(0.0f, size.x, 0.0f, size.y, 0.0f, 1.0f);
    }

    /**
     * Get the specified anchor location, this accounts for padding.
     * @param anchor Location
     * @param elementSize If an element size is given, the aligned position is returned.
     */
    [[nodiscard]] glm::vec2 getAnchorPosition(const Anchor anchor, const std::optional<glm::vec2>& elementSize = std::nullopt) const noexcept
    {
        const glm::vec2 center = size/ 2.0f;
        const glm::vec2 offset = elementSize.value_or(glm::vec2(0.0f));

        using enum Anchor;
        switch (anchor)
        {
            case TopLeft:       return { padding.x + offset.x, padding.y + offset.y };
            case TopCenter:     return { center.x, padding.y + offset.y };
            case TopRight:      return { size.x - padding.x - offset.x, padding.y + offset.y };
            case MiddleLeft:    return { padding.x + offset.x, center.y };
            case Center:        return center;
            case MiddleRight:   return { size.x - padding.x - offset.x, center.y };
            case BottomLeft:    return { padding.x + offset.x, size.y - padding.y - offset.y };
            case BottomCenter:  return { center.x, size.y - padding.y - offset.y };
            case BottomRight:   return { size.x - padding.x - offset.x, size.y - padding.y - offset.y };
        }

        std::unreachable();
    }
};

struct Quad
{
    glm::vec2 center;
    glm::vec2 sideLengths;

    glm::vec4 backgroundColor;
    int32_t   textureIndex = -1;

    [[nodiscard]] UIGeometry toUIGeometry() const noexcept
    {
        const glm::vec2 h = sideLengths * 0.5f;

        const float l = center.x - h.x;
        const float r = center.x + h.x;
        const float t = center.y - h.y;
        const float b = center.y + h.y;

        UIGeometry geom;
        geom.vertices = {
            {{ l, t }, { 0.0f, 0.0f }},     // top-left
            {{ r, t }, { 1.0f, 0.0f }},     // top-right
            {{ r, b }, { 1.0f, 1.0f }},     // bottom-right
            {{ l, b }, { 0.0f, 1.0f }},     // bottom-left
        };
        geom.indices = { 0, 1, 2, 0, 2, 3 };
        return geom;
    }
};

struct TextElement
{
    std::string text;
    glm::vec2   position;
    glm::vec4   color;
    uint32_t    indexOffset;
    uint32_t    indexCount;
    FontFace*   pFont;
};

class TitleScreen
{
    struct PushConstant
    {
        glm::mat4 proj;
        glm::vec4 color;
        int32_t   textureIndex = -1;
        int32_t   isText = 0;
    };
public:
    explicit TitleScreen(const glm::vec2& size, const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager);

    void render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData);

private:
    UIScreen                    mScreen;

    UPtr<FontFace>              mFont48;
    UPtr<FontFace>              mFont16;
    UPtr<FontFace>              mExodus;
    std::vector<TextElement>    mText;

    std::vector<Quad>           mQuads;
    SPtr<RHI::Buffer>           mVertices;
    SPtr<RHI::Buffer>           mIndices;

    SPtr<RHI::VulkanRHI>        mRHI;
    TextureManager*             mTextureManager;

    SPtr<RHI::RenderPass>       mRenderPass;
    SPtr<RHI::Pipeline>         mPipeline;
    SPtr<RHI::Image>            mResult;
};
