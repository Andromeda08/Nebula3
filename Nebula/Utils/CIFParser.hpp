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
	std::string compId;
	std::string atom2Id;
	BondType bondType;
};

/*
* Observed: Experimentally determined position
* Expected: Theoretically ideal position
* Expected is 0 or the fractal coordinates, if no ideal position was stored in the file
 */
struct PositionData
{
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

template <typename T> using CIFContainer1 = std::unordered_map<std::string, std::vector<T>>;
template <typename T> using CIFContainer2 = std::unordered_map<std::string, std::unordered_map<std::string, std::vector<T>>>;

class CIFParser
{
public:
	CIFParser(const std::string& filename);

	void printAtoms();
	void printBonds();
	void printPositions();

	inline bool hasFractTransfData() const { return m_fractTransfData; }

	CIFContainer1<AtomData> atoms;
	CIFContainer2<BondData> bonds;
	CIFContainer2<PositionData> positions;
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
};

