#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <gemmi/cif.hpp>
#include <glm/glm.hpp>

// If defined, converts cartesian to fractional coords and stores those in the expected field of PositionData instead of 0
#define CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES

enum class BondType
{
    sing = 1,
    doub,
    trip,
    quad,
    arom,
    poly,
    delo,
    pi
};

struct AtomData
{
    std::string compId;
    std::string atomId;
    std::string typeSymbol;
};

struct BondData
{
    std::string atom2Id;
    BondType bondType;
};

struct AtomSiteId
{
    AtomSiteId& setAtomId(const std::string& _atomId) noexcept
    {
        atomId = _atomId;
        return *this;
    }

    auto operator<=>(const AtomSiteId&) const = default;

    std::string asymId;
    std::string seqId;
    std::string compId;
    std::string atomId;
};

//https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
struct AtomSiteHasher
{
    std::size_t operator()(const AtomSiteId& id) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        size_t res = 17;
        res = res * 31 + hash<string>()(id.asymId);
        res = res * 31 + hash<string>()(id.seqId);
        res = res * 31 + hash<string>()(id.compId);
        res = res * 31 + hash<string>()(id.atomId);
        return res;
    }
};

/*
* Observed: Experimentally determined position
* Expected: Theoretically ideal position
* Expected is 0 or the fractal coordinates, if no ideal position was stored in the file
 */
struct PositionData
{
    glm::vec3   observed;
    glm::vec3   expected;
    std::string typeSymbol;
};

#ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
// Only exists if fractional transforms are saved in the file (m_fractTransfData = true)
struct FractionalTransform
{
    glm::mat3 transfMatrix;
    glm::vec3 translVector;
};
#endif

using CompoundId = std::string;

using AtomContainer = std::unordered_map<CompoundId, std::vector<AtomData>>;
using BondContainer = std::unordered_map<CompoundId, std::unordered_map<std::string, std::vector<BondData>>>;
using AtomSiteContainer = std::unordered_map<AtomSiteId, PositionData, AtomSiteHasher>;

class CIFParser
{
public:
    explicit CIFParser(const std::string& filename, bool centerPos = false);

    void printAtoms() const noexcept;
    void printBonds() const noexcept;
    void printPositions() const noexcept;

    bool hasFractTransfData() const { return mFractTransfData; }

    AtomContainer       mAtoms;
    BondContainer       mBonds;
    AtomSiteContainer	mPositions;
    glm::vec3			mCenterOffset;

    #ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
    FractionalTransform mFractTrans;
    #endif

private:
    void loadCIFFile(const std::string& filename);
    void parseAtoms();
    void parseBonds();
    void getPositions(gemmi::cif::Table& table, gemmi::cif::Column& compIds, gemmi::cif::Column& atomIds);

    gemmi::cif::Block mBlock;
    bool mUseAtomSize;
    bool mFractTransfData;
    bool mCenterPos;
};

