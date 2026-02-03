#include "CIFParser.hpp"

#include <algorithm>
#include <print>
#include <string>

#include <gemmi/numb.hpp>
#include <gemmi/symmetry.hpp>

#include "Core/Ranges.hpp"
#include "Core/ToString.hpp"
#include "Core/Util.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace cif = gemmi::cif;

// mmCIF constants
#pragma region

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

#pragma endregion

CIFParser::CIFParser(const std::string& filename, const bool centerPos)
: mCenterPos{ centerPos }
{
    loadCIFFile(filename);
}

void CIFParser::printAtoms() const noexcept
{
    std::println("Atoms: {}", mAtoms.size());
    for (const auto& [k, v] : mAtoms)
    {
        for (const auto& e : v)
        {
            std::println("{}: {}, {}", k, e.atomId, e.typeSymbol);
        }
    }
}

void CIFParser::printBonds() const noexcept
{
    std::println("Bonds: {}", mBonds.size());
    for (const auto& [compoundId, v2] : mBonds)
    {
        for (const auto& [atomId, v] : v2)
        {
            for (const auto& bondData : v)
            {
                std::println("{} [{}]: {}, {}",
                    compoundId, atomId, bondData.atom2Id, static_cast<int32_t>(bondData.bondType));
            }
        }
    }
}

void CIFParser::printPositions() const noexcept
{
    std::println("Positions (Observed | Expected):");
    for (const auto& [k, v] : mPositions)
    {
        std::println("{}: {} | {}", k.atomId, fmt::vec(v.observed), fmt::vec(v.expected));
    }
}

void CIFParser::loadCIFFile(const std::string& filename)
{
    cif::Document file = cif::read_file(filename);
    try
    {
        mBlock = file.sole_block();
    }
    catch (const std::exception& _)
    {
        exitWithError("Unsupported CIF file: The file contains more than one sole data block!");
    }

    auto mmCIFCategories = mBlock.get_mmcif_category_names();
    const auto hasMandatoryCategories = std::ranges::all_of(MANDATORY_MMCIF_CATEGORIES, [&mmCIFCategories](const char* category) -> bool {
        return contains(mmCIFCategories, category);
    });
    exitOnAssert(hasMandatoryCategories, "Unsupported CIF file: The file is missing mandatory categories!");

    mUseAtomSize     = contains(mmCIFCategories, OPTIONAL_MMCIF_CATEGORIES[0]);
    mFractTransfData = contains(mmCIFCategories, OPTIONAL_MMCIF_CATEGORIES[1]);
    mCenterOffset    = glm::vec3(0.0f);

    parseAtoms();
    parseBonds();
}

void CIFParser::parseAtoms()
{
    constexpr std::string atomCategory = MANDATORY_MMCIF_CATEGORIES[1];
    auto table = mBlock.find_mmcif_category(atomCategory);
    exitOnAssert(table.ok(), "{} data is empty!", atomCategory);

    auto compIds     = table.find_column("comp_id");
    auto atomIds     = table.find_column("atom_id");
    auto typeSymbols = table.find_column("type_symbol");

    for (auto i = 0; i < table.length(); i++)
    {
        mAtoms[compIds[i]].emplace_back(compIds[i], atomIds[i], typeSymbols[i]);
    }

    getPositions(table, compIds, atomIds);
}

void CIFParser::parseBonds()
{
    constexpr std::string bondCategory = MANDATORY_MMCIF_CATEGORIES[2];
    auto table = mBlock.find_mmcif_category(bondCategory);
    exitOnAssert(table.ok(), "{} data is empty!", bondCategory);

    auto compIds   = table.find_column("comp_id");
    auto atom1Ids  = table.find_column("atom_id_1");
    auto atom2Ids  = table.find_column("atom_id_2");
    auto bondTypes = table.find_column("value_order");

    for (auto i = 0; i < table.length(); i++)
    {
        auto bond = bondTypes[i];
        std::ranges::transform(bond, bond.begin(), [](const unsigned char c) { return std::tolower(c); });
        mBonds[compIds[i]][atom1Ids[i]].emplace_back(atom2Ids[i], BOND_TYPE_CONV_TABLE.at(bond));
    }
}

namespace detail
{
    [[nodiscard]] inline glm::vec3 makeVec3(const std::array<std::string, 3>& columnValues) noexcept
    {
        return glm::vec3(
            cif::as_number(columnValues[0]),
            cif::as_number(columnValues[1]),
            cif::as_number(columnValues[2])
        );
    }

    void updateCenterOffsetInplace(glm::vec3& centerOffset, const glm::vec3& newPosition, const int32_t i) noexcept
    {
        centerOffset += (1.0f / (i + 1.0f)) * (newPosition - centerOffset);
    }
}

void CIFParser::getPositions(cif::Table& table, cif::Column& compIds, cif::Column& atomIds)
{
    // model_cartn_xyz:			   Experimentally determined coordinates
    // pdbx_model_cartn_xyz_ideal: Expected coordinates
    if (!mUseAtomSize)
    {
        auto cartnX = table.find_column("model_Cartn_x");
        auto cartnY = table.find_column("model_Cartn_y");
        auto cartnZ = table.find_column("model_Cartn_z");
        auto idealX = table.find_column("pdbx_model_Cartn_x_ideal");
        auto idealY = table.find_column("pdbx_model_Cartn_y_ideal");
        auto idealZ = table.find_column("pdbx_model_Cartn_z_ideal");

        for (auto i = 0; i < table.length(); i++)
        {
            const auto observedPosition = detail::makeVec3({cartnX[i], cartnY[i], cartnZ[i]});
            if (mCenterPos)
            {
                detail::updateCenterOffsetInplace(mCenterOffset, observedPosition, i);
            }

            // "" Non-existent in this case, unique id possible with compId and atomId.
            AtomSiteId key = { "", "", compIds[i], atomIds[i] };
            mPositions[key] = PositionData {
                .observed = observedPosition,
                .expected = detail::makeVec3({idealX[i], idealY[i], idealZ[i]}),
                .typeSymbol = mAtoms[compIds[i]][i].typeSymbol,
            };
        }

        if (mCenterPos)
        {
            for (auto& v : mPositions | std::views::values)
            {
                v.observed -= mCenterOffset;
            }
        }
    }
    else
    {
        constexpr std::string atomSiteCategory = OPTIONAL_MMCIF_CATEGORIES[0];
        auto siteTable = mBlock.find_mmcif_category(atomSiteCategory);
        auto siteCompIds = siteTable.find_column("label_comp_id");
        auto siteAtomIds = siteTable.find_column("label_atom_id");
        auto siteSeqIds  = siteTable.find_column("label_seq_id");
        auto siteAsymIds = siteTable.find_column("label_asym_id");
        auto cartnX      = siteTable.find_column("Cartn_x");
        auto cartnY      = siteTable.find_column("Cartn_y");
        auto cartnZ      = siteTable.find_column("Cartn_z");

        for (auto i = 0; i < siteTable.length(); i++)
        {
            const auto observedPosition = detail::makeVec3({cartnX[i], cartnY[i], cartnZ[i]});
            if (mCenterPos)
            {
                detail::updateCenterOffsetInplace(mCenterOffset, observedPosition, i);
            }
            AtomSiteId id = { siteAsymIds[i], siteSeqIds[i], siteCompIds[i], siteAtomIds[i] };
            mPositions[id] = PositionData {
                .observed = observedPosition,
                .expected = glm::vec3(0.0f),
                .typeSymbol = "",
            };
        }

        if (mCenterPos)
        {
            for (auto& v : mPositions | std::views::values)
            {
                v.observed -= mCenterOffset;
            }
        }

        #ifdef CIF_PARSER_PRODUCE_FRACTIONAL_COORDINATES
        if (mFractTransfData)
        {
            constexpr std::string atomSitesCategory = OPTIONAL_MMCIF_CATEGORIES[1];
            auto sitesTable = mBlock.find_mmcif_category(atomSitesCategory);

            for (auto i = 0; i < 3; i++)
            {
                for (auto j = 0; j < 3; j++)
                {

                    auto matEntry = sitesTable.find_column(std::format("fract_transf_matrix[{}][{}]", i + 1, j + 1));
                    mFractTrans.transfMatrix[j][i] = cif::as_number(matEntry[0]);
                }

                auto vecEntry = sitesTable.find_column(std::format("fract_transf_vector[{}]", i + 1));
                mFractTrans.translVector[i] = cif::as_number(vecEntry[0]);
            }
        }
        #endif
    }
}
