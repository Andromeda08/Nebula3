#pragma once

#include <string>
#include <array>
#include <unordered_map>
#include <vector>

#include <gemmi/cif.hpp>

//using array_xyz = std::array<double, 3>;

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

struct PositionData
{
	PositionData(double ox, double oy, double oz, double ex, double ey, double ez) {
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

template<typename T>
using CIFContainer = std::unordered_map<std::string, std::vector<T>>;

class CIFParser
{
public:
	CIFParser(const std::string& filename);

	void printAtoms();
	void printBonds();
	void printPositions();

	CIFContainer<AtomData> atoms;
	CIFContainer<BondData> bonds;
	CIFContainer<PositionData> positions;

private:
	void loadCIFFile(const std::string& filename);
	//array_xyz applySymmetryXYZ(const array_xyz& fracts, const std::string& symmetry);
	void getAtoms();
	void getBonds();
	void getPositions(gemmi::cif::Table& table, gemmi::cif::Column& atomIds);

	gemmi::cif::Block m_block;
	bool m_useAtomSite;
};

