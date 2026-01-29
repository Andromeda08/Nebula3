#pragma once

#include <string>
#include <vector>
#include "CIFGeometry.hpp"
#include "CIFParser.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
	class StructurePass;
}

struct MoleculeInfo {
	size_t atoms;
	size_t bonds;
	size_t vertices;
};

struct CIFDataCreateInfo {
	std::string			 filename;
	bool				 centerMolecule;
	SPtr<RHI::VulkanRHI> rhi;
};

class CIFData {
public:
	nbl_CTOR(CIFData);

	const std::vector<glm::vec3>& getAtomPositions() const noexcept { return mAtomPositions; }
	const geo::CIFInstanceData&	getCIFInstanceData() const noexcept { return mCID; }
	const MoleculeInfo& getInfo() const noexcept { return mInfo; }

	const std::string& getName() const noexcept { return mName; }

private:
	friend class Molecule::StructurePass;

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
	/*SPtr<RHI::Buffer>		mSDFAtomPositionsBuffer;*/

	SPtr<RHI::VulkanRHI>	mRHI;
	MoleculeInfo			mInfo;
	std::string				mName;
};