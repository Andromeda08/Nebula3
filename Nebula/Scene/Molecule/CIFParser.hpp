#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <gemmi/cif.hpp>

// If defined, uses glm datatypes instead of std datatypes
#define USE_GLM

// If defined, converts cartesian to fractional coords and stores those in the expected field of PositionData instead of 0
#define CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES

#ifndef USE_GLM
#include <array>
#else
#include <glm/glm.hpp>
#endif

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
	AtomSiteId(const std::string& asymId, const std::string& seqId, const std::string& compId, const std::string& atomId)
		: asymId{ asymId }, seqId{ seqId }, compId{ compId }, atomId{ atomId }
	{
	}

	friend std::ostream& operator<<(std::ostream& os, const AtomSiteId& id) {
		os << id.compId << ", " << id.atomId << ", " << id.asymId << ", " << id.seqId;
		return os;
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
	PositionData() = default;

	PositionData(double ox, double oy, double oz, double ex = 0, double ey = 0, double ez = 0) {
		observed[0] = ox;
		observed[1] = oy;
		observed[2] = oz;
		expected[0] = ex;
		expected[1] = ey;
		expected[2] = ez;
	}
#ifndef USE_GLM
	std::array<double, 3> observed;
	std::array<double, 3> expected;
#else
	glm::vec3 observed;
	glm::vec3 expected;
#endif
};

#ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
// Only exists if fractional transforms are saved in the file (m_fractTransfData = true)
struct FractionalTransform
{
#ifndef USE_GLM
	std::array<double, 9> transfMatrix;
	std::array<double, 3> translVector;
#else
	glm::mat3 transfMatrix;
	glm::vec3 translVector;
#endif
};
#endif

using AtomContainer = std::unordered_map<std::string, std::vector<AtomData>>;
using BondContainer = std::unordered_map<std::string, std::unordered_map<std::string, std::vector<BondData>>>;
using AtomSiteContainer = std::unordered_map<AtomSiteId, PositionData, AtomSiteHasher>;

class CIFParser
{
public:
	CIFParser(const std::string& filename, bool centerPos = false);

	void printAtoms();
	void printBonds();
	void printPositions();

	inline bool hasFractTransfData() const { return m_fractTransfData; }

	AtomContainer atoms;
	BondContainer bonds;
	AtomSiteContainer positions;
#ifdef USE_GLM
	glm::vec3 centerOffset;
#endif
#ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
	FractionalTransform fractTrans;
#endif

private:
	void loadCIFFile(const std::string& filename);
	void getAtoms();
	void getBonds();
	void getPositions(gemmi::cif::Table& table, gemmi::cif::Column& compIds, gemmi::cif::Column& atomIds);

	gemmi::cif::Block m_block;
	bool m_useAtomSite;
	bool m_fractTransfData;
	bool m_centerPos;
};

