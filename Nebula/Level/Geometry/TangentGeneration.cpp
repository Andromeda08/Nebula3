#include "TangentGeneration.hpp"

#include <spdlog/spdlog.h>

namespace nbl::Tangent
{
    int getNumFaces(const SMikkTSpaceContext* pContext)
    {
        const auto* data = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        return static_cast<int>(data->indices->size() / 3);
    }

    int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
    {
        return 3;
    }

    void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec3& pos   = (*data->positions)[index];

        fvPosOut[0] = pos.x;
        fvPosOut[1] = pos.y;
        fvPosOut[2] = pos.z;
    }

    void getNormal(const SMikkTSpaceContext* pContext, float fvNormalOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec3& norm  = (*data->normals)[index];

        fvNormalOut[0] = norm.x;
        fvNormalOut[1] = norm.y;
        fvNormalOut[2] = norm.z;
    }

    void getTexCoord(const SMikkTSpaceContext* pContext, float fvUVOut[], const int iFace, const int iVert)
    {
        const auto*      data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t   index = (*data->indices)[iFace * 3 + iVert];
        const glm::vec2& uv    = (*data->texcoords)[index];

        fvUVOut[0] = uv.x;
        fvUVOut[1] = uv.y;
    }

    void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace,
        const int iVert)
    {
        const auto*    data  = static_cast<MikkTSpaceContext*>(pContext->m_pUserData);
        const uint32_t index = (*data->indices)[iFace * 3 + iVert];

        glm::vec4& tangent = (*data->tangents)[index];
        tangent            = {
            fvTangent[0], fvTangent[1], fvTangent[2],
            (fSign >= 0.0f) ? 1.0f : -1.0f,
        };
    }

    void generateTangents(const uint32_t vertexCount, const MikkTSpaceContext& context)
    {
        MikkTSpaceContext _context = context;
        context.tangents->resize(vertexCount);

        SMikkTSpaceInterface interface {};
        interface.m_getNumFaces          = getNumFaces;
        interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
        interface.m_getPosition          = getPosition;
        interface.m_getNormal            = getNormal;
        interface.m_getTexCoord          = getTexCoord;
        interface.m_setTSpaceBasic       = setTSpaceBasic;

        SMikkTSpaceContext ctx {};
        ctx.m_pInterface = &interface;
        ctx.m_pUserData  = &_context;

        if (!genTangSpaceDefault(&ctx))
        {
            spdlog::warn("Failed to generate tangents for primitive");
        }
    }
}
