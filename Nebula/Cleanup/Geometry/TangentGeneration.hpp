#pragma once

#include <mikktspace.h>
#include <vector>
#include <glm/glm.hpp>
#include "../VertexTypes.hpp"

namespace nbl::Tangent
{
    struct MikkTSpaceContext
    {
        std::vector<glm::vec3>* positions;
        std::vector<glm::vec3>* normals;
        std::vector<glm::vec2>* texcoords;
        std::vector<uint32_t>*  indices;

        std::vector<glm::vec4>* tangents;
    };

    int getNumFaces(const SMikkTSpaceContext* pContext);

    int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, int iFace);

    void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], int iFace, int iVert);

    void getNormal(const SMikkTSpaceContext* pContext, float fvNormalOut[], int iFace, int iVert);

    void getTexCoord(const SMikkTSpaceContext* pContext, float fvUVOut[], int iFace, int iVert);

    void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], float fSign, int iFace, int iVert);

    void generateTangents(uint32_t vertexCount, const MikkTSpaceContext& context);
}
