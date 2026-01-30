#include "CIFData.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>

namespace detail
{
    [[nodiscard]] [[maybe_unused]] constexpr glm::vec4 getAtomColor(const std::string& typeSymbol) noexcept
    {
        if (typeSymbol == "H")
        {
            return glm::vec4(0.95f, 0.1f, 0.1f, 1.0f);
        }
        return glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    }
}

CIFData::CIFData(const CIFDataCreateInfo& createInfo)
: mCIF(createInfo.filename, createInfo.centerMolecule)
, mRHI(createInfo.rhi)
{
    mName = std::filesystem::path(createInfo.filename).stem().string();
    // spdlog::debug("Loaded molecule: {}", styled(mName, fg(fmt::color::cyan) | fmt::emphasis::bold));

    mSphereData = mCID.sphere;
    mCylinderData = mCID.cylinder;

    for (const auto& v : mCIF.mPositions | std::views::values)
    {
        auto t = Transform().setScale(glm::vec3(0.25f)).setTranslate(v.observed);
        const auto m = t.getModel();

        mCID.sphereTransforms.push_back(m);
        mAtomPositions.push_back(v.observed);
    }

    for (const auto& [k, v] : mCIF.mPositions)
    {
        if (!mCIF.mBonds.contains(k.compId))
        {
            continue;
        }

        auto atoms1 = mCIF.mBonds.at(k.compId);
        if (!atoms1.contains(k.atomId))
        {
            continue;
        }

        for (const auto& bondData : atoms1.at(k.atomId))
        {
            AtomSiteId id = AtomSiteId(k).setAtomId(bondData.atom2Id);

            auto a2it = mCIF.mPositions.find(id);
            if (a2it == mCIF.mPositions.end())
            {
                continue;
            }

            glm::vec3 atom1 = v.observed;
            glm::vec3 atom2 = a2it->second.observed;

            const auto bondTransform = geo::calcBondTransforms(atom1, atom2);
            auto t = Transform()
                .setScale(glm::vec3(1.0f, bondTransform.dist, 1.0f))
                .setTranslate(bondTransform.position)
                .setAxisAngleRotation(bondTransform.axis, bondTransform.angle);
            const auto m = t.getModel();

            mCID.cylinderTransforms.push_back(m);
        }
    }

    mInfo.atoms = mCID.sphereTransforms.size();
    mInfo.bonds = mCID.cylinderTransforms.size();
    mInfo.vertices = mSphereData.vertices.size() + mCylinderData.vertices.size();

    createRenderingResources();
}

void CIFData::createRenderingResources() noexcept
{
    /* Sphere Geometry & Instances */ {
        const auto vertexSize = mSphereData.vertices.size() * sizeof(glm::vec3);
        mSphereVertexBuffer = mRHI->createBuffer({
            .size  = vertexSize,
            .type  = RHI::BufferType::Vertex,
            .label = "Sphere-VertexBuffer",
        });

        const auto indexSize = mSphereData.indices.size() * sizeof(uint32_t);
        mSphereIndexBuffer = mRHI->createBuffer({
            .size  = indexSize,
            .type  = RHI::BufferType::Index,
            .label = "Sphere-IndexBuffer",
        });

        const auto instanceSize = mCID.sphereTransforms.size() * sizeof(glm::mat4);
        mSphereInstanceBuffer = mRHI->createBuffer({
            .size  = instanceSize,
            .type  = RHI::BufferType::Vertex,
            .label = "Sphere-InstanceBuffer"
        });

        const auto stagingBuffer = mRHI->createBuffer({
            .size  = vertexSize + indexSize + instanceSize,
            .type  = RHI::BufferType::Staging,
            .label = "Sphere-Staging",
        });
        stagingBuffer->setData(mSphereData.vertices.data(), vertexSize, 0);
        stagingBuffer->setData(mSphereData.indices.data(), indexSize, vertexSize);
        stagingBuffer->setData(mCID.sphereTransforms.data(), instanceSize, vertexSize + indexSize);

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            const auto vertexRegion = vk::BufferCopy2().setSrcOffset(0).setDstOffset(0).setSize(vertexSize);
            const auto vertexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mSphereVertexBuffer->getHandle())
                .setRegions(vertexRegion);
            commandList->getHandle().copyBuffer2(vertexCopy);

            const auto indexRegion = vk::BufferCopy2().setSrcOffset(vertexSize).setDstOffset(0).setSize(indexSize);
            const auto indexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mSphereIndexBuffer->getHandle())
                .setRegions(indexRegion);
            commandList->getHandle().copyBuffer2(indexCopy);

            const auto instanceRegion = vk::BufferCopy2().setSrcOffset(vertexSize + indexSize).setDstOffset(0).setSize(instanceSize);
            const auto instanceCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mSphereInstanceBuffer->getHandle())
                .setRegions(instanceRegion);
            commandList->getHandle().copyBuffer2(instanceCopy);
        });
    }

    /* Cylinder Geometry & Instances */ {
        const auto vertexSize = mCylinderData.vertices.size() * sizeof(glm::vec3);
        mCylinderVertexBuffer = mRHI->createBuffer({
            .size  = vertexSize,
            .type  = RHI::BufferType::Vertex,
            .label = "Cylinder-VertexBuffer",
        });

        const auto indexSize = mCylinderData.indices.size() * sizeof(uint32_t);
        mCylinderIndexBuffer = mRHI->createBuffer({
            .size  = indexSize,
            .type  = RHI::BufferType::Index,
            .label = "Cylinder-IndexBuffer",
        });

        const auto instanceSize = mCID.cylinderTransforms.size() * sizeof(glm::mat4);
        mCylinderInstanceBuffer = mRHI->createBuffer({
            .size  = instanceSize,
            .type  = RHI::BufferType::Vertex,
            .label = "Cylinder-InstanceBuffer"
        });

        const auto stagingBuffer = mRHI->createBuffer({
            .size  = vertexSize + indexSize + instanceSize,
            .type  = RHI::BufferType::Staging,
            .label = "Cylinder-Staging",
        });
        stagingBuffer->setData(mCylinderData.vertices.data(), vertexSize, 0);
        stagingBuffer->setData(mCylinderData.indices.data(), indexSize, vertexSize);
        stagingBuffer->setData(mCID.cylinderTransforms.data(), instanceSize, vertexSize + indexSize);

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            const auto vertexRegion = vk::BufferCopy2().setSrcOffset(0).setDstOffset(0).setSize(vertexSize);
            const auto vertexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mCylinderVertexBuffer->getHandle())
                .setRegions(vertexRegion);
            commandList->getHandle().copyBuffer2(vertexCopy);

            const auto indexRegion = vk::BufferCopy2().setSrcOffset(vertexSize).setDstOffset(0).setSize(indexSize);
            const auto indexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mCylinderIndexBuffer->getHandle())
                .setRegions(indexRegion);
            commandList->getHandle().copyBuffer2(indexCopy);

            const auto instanceRegion = vk::BufferCopy2().setSrcOffset(vertexSize + indexSize).setDstOffset(0).setSize(instanceSize);
            const auto instanceCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mCylinderInstanceBuffer->getHandle())
                .setRegions(instanceRegion);
            commandList->getHandle().copyBuffer2(instanceCopy);
        });
    }
}
