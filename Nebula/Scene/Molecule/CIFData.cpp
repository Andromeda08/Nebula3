#include "CIFData.hpp"

CIFData::CIFData(const CIFDataCreateInfo& createInfo)
: mCIF(createInfo.filename, createInfo.centerMolecule)
{
    for (const auto& [k, v] : mCIF.positions) {
        mSpheres.push_back(geom::createSphereData(.1f, 8, 8));
        mSpheres.back().setPosition(v.observed);
        mAtomPositions.push_back(v.observed);
    }

    for (const auto& [k, v] : mCIF.positions) {
        auto atoms1 = mCIF.bonds.at(k.compId);
        if (atoms1.find(k.atomId) == atoms1.end()) continue;
        for (const auto& e : atoms1.at(k.atomId)) {
            AtomSiteId id = { k.asymId, k.seqId, k.compId, e.atom2Id };
            auto a2it = mCIF.positions.find(id);
            if (a2it == mCIF.positions.end()) continue;
            glm::vec3 atom1 = v.observed;
            glm::vec3 atom2 = a2it->second.observed;
            auto bt = geom::calcBondTransforms(atom1, atom2);
            mCylinders.push_back(geom::createCylinderData(8, .05, bt.dist));
            mCylinders.back().setRotation(bt.axis, bt.angle);
            mCylinders.back().setPosition(bt.position);
        }
    }
}