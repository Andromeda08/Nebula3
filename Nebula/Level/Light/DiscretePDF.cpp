#include "DiscretePDF.hpp"

#include <ranges>
#include "Level/Geometry/Geometry.hpp"

namespace nbl
{
    DiscretePDF::DiscretePDF(const size_t nItems)
    {
        reserve(nItems);
        clear();
    }

    DiscretePDF::DiscretePDF(const Geometry* pGeometry)
    {
        clear();
        reserve(pGeometry->getTriangleCount());
        for (size_t i = 0; i < pGeometry->getTriangleCount(); i++)
        {
            append(pGeometry->getTriangleArea(i));
        }
        normalize();
    }

    void DiscretePDF::reserve(const size_t nItems)
    {
        mCDF.reserve(nItems + 1);
    }

    void DiscretePDF::clear()
    {
        mCDF.clear();
        mCDF.push_back(0.0f);
        mIsNormalized = false;
    }

    void DiscretePDF::append(const float pdfValue)
    {
        mCDF.push_back(mCDF[mCDF.size() - 1] + pdfValue);
    }

    size_t DiscretePDF::size() const
    {
        return mCDF.size() - 1;
    }

    float DiscretePDF::operator[](const size_t entry) const
    {
        return mCDF[entry + 1] - mCDF[entry];
    }

    bool DiscretePDF::isNormalized() const
    {
        return mIsNormalized;
    }

    float DiscretePDF::getSum() const
    {
        return mSum;
    }

    float DiscretePDF::getNormalization() const
    {
        return mNormalization;
    }

    float DiscretePDF::normalize()
    {
        mSum = mCDF[mCDF.size() - 1];

        if (mSum > 0.0f)
        {
            mNormalization = 1.0f / mSum;
            for (size_t i = 1; i < mCDF.size(); ++i)
            {
                mCDF[i] *= mNormalization;
            }
            mCDF[mCDF.size() - 1] = 1.0f;
            mIsNormalized         = true;
        }
        else
        {
            mNormalization = 0.0f;
        }
        return mSum;
    }

    size_t DiscretePDF::sample(const float sampleValue) const
    {
        const auto entry = std::ranges::lower_bound(mCDF, sampleValue);
        const auto index = static_cast<size_t>(std::max(static_cast<std::ptrdiff_t>(0), entry - mCDF.begin() - 1));
        return std::min(index, mCDF.size() - 2);
    }

    size_t DiscretePDF::sample(const float sampleValue, float& pdf) const
    {
        size_t index = sample(sampleValue);
        pdf          = operator[](index);
        return index;
    }
}
