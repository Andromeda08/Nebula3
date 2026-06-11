#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include <stb_image.h>
#include <freetype/freetype.h>
#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>

#include "UIGeometry.hpp"
#include "Scene/TextureManager.hpp"

struct FontGlyph
{
    glm::vec2   size;
    glm::vec2   bearing;
    uint32_t    advance;
    glm::vec2   uvMin;
    glm::vec2   uvMax;
};

struct TextBounds
{
    float width   = 0.0f;
    float ascent  = 0.0f;
    float descent = 0.0f;
};

class FontFace
{
    static constexpr int32_t sPadding    = 1;
    static constexpr int32_t sAtlasWidth = 1024;
public:
    FontFace(TextureManager* pTextureManager, const std::filesystem::path& font, const float fontSize)
    : mPath(font)
    , mTextureManager(pTextureManager)
    , mFontSize(fontSize)
    {
        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            exitWithError("oops");
        }

        FT_Face fontFace;
        if (FT_New_Face(ft, mPath.string().c_str(), 0, &fontFace))
        {
            exitWithError("Failed to load font: {}", mPath.string().c_str());
        }

        FT_Set_Pixel_Sizes(fontFace, 0, mFontSize);

        int32_t x         = 0;
        int32_t y         = 0;
        int32_t rowHeight = 0;
        for (unsigned char c = 32; c < 127; c++)
        {
            if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
            {
                continue;
            }

            const FT_GlyphSlot glyph = fontFace->glyph;
            const int32_t w = glyph->bitmap.width;
            const int32_t h = glyph->bitmap.rows;

            if (x + w + sPadding >= sAtlasWidth)
            {
                x = 0;
                y += rowHeight + sPadding;
                rowHeight = 0;
            }
            x += w + sPadding;
            rowHeight = std::max(rowHeight, h);
        }
        const int32_t atlasHeight = y + rowHeight + sPadding;

        std::vector<stbi_uc> atlas(sAtlasWidth * atlasHeight * 4, 0);
        x = 0;
        y = 0;
        rowHeight = 0;
        for (unsigned char c = 32; c < 127; c++)
        {
            if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
            {
                continue;
            }

            const FT_GlyphSlot g = fontFace->glyph;
            const int w = static_cast<int>(g->bitmap.width);
            const int h = static_cast<int>(g->bitmap.rows);

            if (x + w + sPadding >= sAtlasWidth)
            {
                x = 0;
                y += rowHeight + sPadding;
                rowHeight = 0;
            }

            for (int row = 0; row < h; ++row)
            {
                const unsigned char* src = g->bitmap.buffer + row * g->bitmap.pitch;
                stbi_uc* dst = atlas.data() + ((y + row) * sAtlasWidth + x) * 4;
                for (int col = 0; col < w; ++col)
                {
                    dst[col * 4 + 0] = 255;
                    dst[col * 4 + 1] = 255;
                    dst[col * 4 + 2] = 255;
                    dst[col * 4 + 3] = src[col];
                }
            }

            FontGlyph glyph;
            glyph.size    = { w, h };
            glyph.bearing = { g->bitmap_left, g->bitmap_top };
            glyph.advance = static_cast<unsigned int>(g->advance.x);
            glyph.uvMin   = {
                static_cast<float>(x) / sAtlasWidth,
                static_cast<float>(y) / atlasHeight
            };
            glyph.uvMax   = {
                static_cast<float>(x + w) / sAtlasWidth,
                static_cast<float>(y + h) / atlasHeight
            };

            mGlyphs[c] = glyph;

            x += w + sPadding;
            rowHeight = std::max(rowHeight, h);
        }

        mTextureIndex = mTextureManager->loadTextureFromMemory(
            fmt::format("font_atlas_{}_{}px", mPath.stem().string(), mFontSize),
            atlas.data(), sAtlasWidth, atlasHeight,
            std::nullopt, std::nullopt, vk::Format::eR8G8B8A8Unorm);
        
        FT_Done_Face(fontFace);
        FT_Done_FreeType(ft);
    }

    [[nodiscard]] uint32_t getTextureIndex() const noexcept { return mTextureIndex; }

    [[nodiscard]] UIGeometry buildTextGeometry(const std::string& text, const glm::vec2& position) const
    {
        UIGeometry geo;
        geo.vertices.reserve(text.size() * 4);
        geo.indices.reserve(text.size() * 6);

        const TextBounds bounds = measureTextBounds(text);

        // float penX = position.x - bounds.width * 0.5f;
        // const float penY = position.y + (bounds.ascent - bounds.descent) * 0.5f;

        float penX = position.x;
        float penY = position.y + bounds.ascent;

        for (const char c : text)
        {
            const auto it = mGlyphs.find(static_cast<unsigned char>(c));
            if (it == mGlyphs.end())
            {
                continue;
            }

            const FontGlyph& g = it->second;

            const float x0 = penX + g.bearing.x;
            const float y0 = penY - g.bearing.y;
            const float x1 = x0 + g.size.x;
            const float y1 = y0 + g.size.y;

            if (g.size.x > 0 && g.size.y > 0)
            {
                const uint32_t base = static_cast<uint32_t>(geo.vertices.size());

                geo.vertices.push_back({ { x0, y0 }, { g.uvMin.x, g.uvMin.y } });
                geo.vertices.push_back({ { x1, y0 }, { g.uvMax.x, g.uvMin.y } });
                geo.vertices.push_back({ { x1, y1 }, { g.uvMax.x, g.uvMax.y } });
                geo.vertices.push_back({ { x0, y1 }, { g.uvMin.x, g.uvMax.y } });

                geo.indices.push_back(base + 0);
                geo.indices.push_back(base + 3);
                geo.indices.push_back(base + 2);
                geo.indices.push_back(base + 0);
                geo.indices.push_back(base + 2);
                geo.indices.push_back(base + 1);
            }

            penX += (g.advance >> 6);
        }

        for (const auto& vertex : geo.vertices)
        {
            geo.bounds.expandBy(glm::vec3(vertex.pos, 0.0f));
        }

        return std::move(geo);
    }

    [[nodiscard]] TextBounds measureTextBounds(const std::string_view text) const noexcept
    {
        TextBounds bounds = {};
        for (const char c : text)
        {
            const auto it = mGlyphs.find(static_cast<unsigned char>(c));
            if (it == mGlyphs.end())
            {
                continue;
            }

            const FontGlyph& g = it->second;

            bounds.width   += (g.advance >> 6);
            bounds.ascent  = std::max(bounds.ascent,  static_cast<float>(g.bearing.y));
            bounds.descent = std::max(bounds.descent, static_cast<float>(g.size.y - g.bearing.y));
        }

        return bounds;
    }

private:
    std::filesystem::path               mPath;
    TextureManager*                     mTextureManager;

    float                               mFontSize;
    uint32_t                            mTextureIndex;
    std::unordered_map<char, FontGlyph> mGlyphs;
};
