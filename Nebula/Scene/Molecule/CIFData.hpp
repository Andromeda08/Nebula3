#include "Utils/CIFParser.hpp"
#include "Utils/Geometry.hpp"

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

	std::vector<geom::Data> getSphereData() const noexcept { return mSpheres; }
	std::vector<geom::Data> getCylinderData() const noexcept { return mCylinders; }
	std::vector<glm::vec3> getAtomPositions() const noexcept { return mAtomPositions; }

private:
	CIFParser mCIF;

	std::vector<geom::Data> mSpheres;
	std::vector<geom::Data> mCylinders;
	std::vector<glm::vec3> mAtomPositions;
};