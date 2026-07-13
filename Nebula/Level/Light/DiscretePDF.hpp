#pragma once

#include <vector>

namespace nbl
{
    class Geometry;

    class [[nodiscard]] DiscretePDF
    {
    public:
        explicit DiscretePDF(size_t nItems = 0);

        explicit DiscretePDF(const Geometry* pGeometry);

        void reserve(size_t nItems);

        void clear();

        void append(const float pdfValue);

        size_t size() const;

        float operator[](const size_t entry) const;

        bool isNormalized() const;

        float getSum() const;

        float getNormalization() const;

        float normalize();

        size_t sample(const float sampleValue) const;

        size_t sample(const float sampleValue, float& pdf) const;

        const std::vector<float>& getValues() const { return mCDF; }

    private:
        std::vector<float>  mCDF;
        float               mSum           = 0.0f;
        float               mNormalization = 0.0f;
        bool                mIsNormalized  = false;
    };

    struct GPUDiscretePDF
    {
        float              sum;
        std::vector<float> cdf;
    };
}