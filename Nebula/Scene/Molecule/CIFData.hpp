#include "CIFParser.hpp"
#include "CIFGeometry.hpp"

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include <string>
#include <vector>

struct CIFDataCreateInfo {
	std::string filename;
	bool centerMolecule;
};

class CIFData {
public:
	//nbl_DISABLE_COPY(CIFData);
	//nbl_CTOR(CIFData);

	CIFData(const CIFDataCreateInfo& createInfo);

	const std::vector<geo::Data>&	getSphereData() const noexcept { return mSpheres; }
	const std::vector<geo::Data>&	getCylinderData() const noexcept { return mCylinders; }
	const std::vector<glm::vec3>&	getAtomPositions() const noexcept { return mAtomPositions; }
	const geo::CIFInstanceData&		getCIFInstanceData() const noexcept { return mCID; }

private:
	CIFParser mCIF;

	std::vector<geo::Data> mSpheres;
	std::vector<geo::Data> mCylinders;
	std::vector<glm::vec3> mAtomPositions;

	geo::CIFInstanceData mCID;
};