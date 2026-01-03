#pragma once

#include <string>
#include <array>
#include <unordered_map>
#include <vector>

#include <gemmi/cif.hpp>

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

	std::array<double, 3> observed;
	std::array<double, 3> expected;
};

#ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
// Only exists if fractional transforms are saved in the file (m_fractTransfData = true)
struct FractionalTransform
{
	std::array<double, 9> transfMatrix;
	std::array<double, 3> translVector;
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
	//array_xyz applySymmetryXYZ(const array_xyz& fracts, const std::string& symmetry);
	void getAtoms();
	void getBonds();
	void getPositions(gemmi::cif::Table& table, gemmi::cif::Column& compIds, gemmi::cif::Column& atomIds);

	gemmi::cif::Block m_block;
	bool m_useAtomSite;
	bool m_fractTransfData;
};

