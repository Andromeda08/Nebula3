#pragma once

#include <string>
#include <vector>
#include "CIFGeometry.hpp"
#include "CIFParser.hpp"
#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct CIFDataCreateInfo {
	std::string			 filename;
	bool				 centerMolecule;
	SPtr<RHI::VulkanRHI> rhi;
};

class CIFData {
public:
	explicit CIFData(const CIFDataCreateInfo& createInfo);

	const std::vector<glm::vec3>& getAtomPositions() const noexcept { return mAtomPositions; }
	const geo::CIFInstanceData&	getCIFInstanceData() const noexcept { return mCID; }

private:
	friend class Scene;

	void createRenderingResources() noexcept;

	CIFParser				mCIF;
	geo::CIFInstanceData	mCID;

	// Spheres for Structure Rendering
	geo::Data				mSphereData;
	SPtr<RHI::Buffer>		mSphereVertexBuffer;
	SPtr<RHI::Buffer>		mSphereIndexBuffer;
	SPtr<RHI::Buffer>		mSphereInstanceBuffer;

	// Cylinders for Structure Rendering
	geo::Data				mCylinderData;
	SPtr<RHI::Buffer>		mCylinderVertexBuffer;
	SPtr<RHI::Buffer>		mCylinderIndexBuffer;
	SPtr<RHI::Buffer>		mCylinderInstanceBuffer;

	// Atom Positions data for SDF
	std::vector<glm::vec3>	mAtomPositions;
	SPtr<RHI::Buffer>		mAtomPositionsBuffer;

	SPtr<RHI::VulkanRHI>	mRHI;
};