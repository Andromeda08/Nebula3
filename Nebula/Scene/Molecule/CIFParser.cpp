#include "CIFParser.hpp"

#include <iostream>
#include <algorithm>

#include <gemmi/symmetry.hpp>
#include <gemmi/numb.hpp>
namespace cif = gemmi::cif;



constexpr std::array MANDATORY_MMCIF_CATEGORIES = {
	"_chem_comp.",
	"_chem_comp_atom.",
	"_chem_comp_bond."
};

constexpr std::array OPTIONAL_MMCIF_CATEGORIES = {
	"_atom_site.",
	"_atom_sites."
};

const std::unordered_map<std::string, BondType> BOND_TYPE_CONV_TABLE = {
	{"sing", BondType::sing},
	{"doub", BondType::doub},
	{"trip", BondType::trip},
	{"quad", BondType::quad},
	{"arom", BondType::arom},
	{"poly", BondType::poly},
	{"delo", BondType::delo},
	{"pi"  , BondType::pi}
};


CIFParser::CIFParser(const std::string& filename, bool centerPos)
	: m_centerPos{ centerPos }
{
	loadCIFFile(filename);
}


void CIFParser::loadCIFFile(const std::string& filename)
{
	cif::Document file = cif::read_file(filename);
	try {
		m_block = file.sole_block();
	}
	catch (std::exception& e) {
		throw std::runtime_error("Unsupported cif file, contains more than one sole data block");
	}

	auto mmcif_categories = m_block.get_mmcif_category_names();
	for (const auto& e : MANDATORY_MMCIF_CATEGORIES) {
		if (std::find(mmcif_categories.begin(), mmcif_categories.end(), e) == mmcif_categories.end()) {
			throw std::runtime_error("Unsupported cif file, does not contain category: " + std::string(e));
		}
	}

	m_useAtomSite = std::find(mmcif_categories.begin(), mmcif_categories.end(), OPTIONAL_MMCIF_CATEGORIES[0]) != mmcif_categories.end();
	m_fractTransfData = std::find(mmcif_categories.begin(), mmcif_categories.end(), OPTIONAL_MMCIF_CATEGORIES[1]) != mmcif_categories.end();
	centerOffset = glm::vec3(0.f);

	getAtoms();
	getBonds();
}

void CIFParser::printAtoms()
{
	std::cout << "Atoms: \n";
	uint64_t cnt = 0;
	for (const auto& [k, v] : atoms) {
		for (const auto& e : v) {
			std::cout << k << ": " << e.atomId << ", " << e.typeSymbol << '\n';
			cnt++;
		}
	}
	std::cout << cnt << std::endl;
}

void CIFParser::printBonds()
{
	std::cout << "Bonds: \n";
	uint64_t cnt = 0;
	for (const auto& [compId, v2] : bonds) {
		for (const auto& [atomId, v] : v2) {
			for (const auto& e : v) {
				std::cout << compId << '[' << atomId << "]: " << e.atom2Id << ", " << static_cast<int>(e.bondType) << '\n';
				cnt++;
			}
		}
	}
	std::cout << cnt << std::endl;
}

void CIFParser::printPositions()
{
	std::cout << "Positions (Observed | Expected): \n";
	for (const auto& [k, v] : positions) {
		std::cout << k << ": " << v.observed[0] << v.observed[1] << v.observed[2]
			<< " | " << v.expected[0] << v.expected[1] << v.expected[2] << '\n';
	}
}

void CIFParser::getAtoms()
{
	const std::string atom_cat = MANDATORY_MMCIF_CATEGORIES[1];
	auto table = m_block.find_mmcif_category(atom_cat);
	if (!table.ok()) {
		throw std::runtime_error(std::string(atom_cat) + " data is empty");
	}

	auto compIds = table.find_column("comp_id");
	auto atomIds = table.find_column("atom_id");
	auto typeSymbols = table.find_column("type_symbol");

	for (auto i = 0; i < table.length(); i++) {
		atoms[compIds[i]].emplace_back(compIds[i], atomIds[i], typeSymbols[i]);
	}

	getPositions(table, compIds, atomIds);
}

void CIFParser::getBonds()
{
	const std::string bond_cat = MANDATORY_MMCIF_CATEGORIES[2];
	auto table = m_block.find_mmcif_category(bond_cat);
	if (!table.ok()) {
		throw std::runtime_error(std::string(bond_cat) + " data is empty");
	}

	auto compIds = table.find_column("comp_id");
	auto atom1Ids = table.find_column("atom_id_1");
	auto atom2Ids = table.find_column("atom_id_2");
	auto bondTypes = table.find_column("value_order");

	for (auto i = 0; i < table.length(); i++) {
		auto bond = bondTypes[i];
		std::transform(bond.begin(), bond.end(), bond.begin(), [](unsigned char c) { return std::tolower(c); });
		bonds[compIds[i]][atom1Ids[i]].emplace_back(atom2Ids[i], BOND_TYPE_CONV_TABLE.at(bond));
	}
}

void CIFParser::getPositions(cif::Table& table, cif::Column& compIds, cif::Column& atomIds)
{
	// model_cartn_xyz: Experimentally determined coordinates
	// pdbx_model_cartn_xyz_ideal: Expected coordinates
	if (!m_useAtomSite) {
		auto cartnX = table.find_column("model_Cartn_x");
		auto cartnY = table.find_column("model_Cartn_y");
		auto cartnZ = table.find_column("model_Cartn_z");
		auto idealX = table.find_column("pdbx_model_Cartn_x_ideal");
		auto idealY = table.find_column("pdbx_model_Cartn_y_ideal");
		auto idealZ = table.find_column("pdbx_model_Cartn_z_ideal");

		for (auto i = 0; i < table.length(); i++) {
			if (m_centerPos) {
				// Hopefully works
				centerOffset += (1.f / (i + 1)) * (glm::vec3(cif::as_number(cartnX[i]), cif::as_number(cartnY[i]), cif::as_number(cartnZ[i])) - centerOffset);
			}
			AtomSiteId key = { "", "", compIds[i], atomIds[i] }; // "" Non existant in this case, unique id possible with compId and atomId
			positions[key] = PositionData(
				cif::as_number(cartnX[i]),
				cif::as_number(cartnY[i]),
				cif::as_number(cartnZ[i]),
				cif::as_number(idealX[i]),
				cif::as_number(idealY[i]),
				cif::as_number(idealZ[i])
			);
		}

		if (m_centerPos) {
			for (auto& [k, v] : positions) {
				v.observed -= centerOffset;
			}
		}
	}
	else {
		const std::string ATOM_SITE_CAT = OPTIONAL_MMCIF_CATEGORIES[0];
		auto siteTable = m_block.find_mmcif_category(ATOM_SITE_CAT);
		auto siteCompIds = siteTable.find_column("label_comp_id");
		auto siteAtomIds = siteTable.find_column("label_atom_id");
		auto siteSeqIds = siteTable.find_column("label_seq_id");
		auto siteAsymIds = siteTable.find_column("label_asym_id");
		auto cartnX = siteTable.find_column("Cartn_x");
		auto cartnY = siteTable.find_column("Cartn_y");
		auto cartnZ = siteTable.find_column("Cartn_z");

		for (auto i = 0; i < siteTable.length(); i++) {
			if (m_centerPos) {
				// Hopefully works
				centerOffset += (1.f / (i + 1)) * (glm::vec3(cif::as_number(cartnX[i]), cif::as_number(cartnY[i]), cif::as_number(cartnZ[i])) - centerOffset);
			}
			AtomSiteId id = { siteAsymIds[i], siteSeqIds[i], siteCompIds[i], siteAtomIds[i] };
			positions[id] = PositionData(
				cif::as_number(cartnX[i]),
				cif::as_number(cartnY[i]),
				cif::as_number(cartnZ[i])
			);
		}

		if (m_centerPos) {
			for (auto& [k, v] : positions) {
				v.observed -= centerOffset;
			}
		}

#ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
		if (m_fractTransfData) {
			const std::string ATOM_SITES_CAT = OPTIONAL_MMCIF_CATEGORIES[1];
			auto sitesTable = m_block.find_mmcif_category(ATOM_SITES_CAT);

			for (auto i = 0; i < 3; i++) {
				for (auto j = 0; j < 3; j++) {
					auto matEntry = sitesTable.find_column("fract_transf_matrix[" + std::to_string(i + 1) + "][" + std::to_string(j + 1) + "]");
#ifndef USE_GLM
					fractTrans.transfMatrix[i * 3 + j] = cif::as_number(matEntry[0]);
#else
					fractTrans.transfMatrix[j][i] = cif::as_number(matEntry[0]);
#endif
				}
				auto vecEntry = sitesTable.find_column("fract_transf_vector[" + std::to_string(i + 1) + "]");
				fractTrans.translVector[i] = cif::as_number(vecEntry[0]);
			}
		}
#endif
	}
}
