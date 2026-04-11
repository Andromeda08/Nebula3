#pragma once

#include "Object.hpp"

namespace nbl
{
    class Scene
    {
    public:
        explicit Scene(const std::string& name)
        : mName(name)
        {
        }

        template <class T, class... Args>
        requires std::is_base_of_v<Object, T>
        T* addObject(const ObjectParams& params, Args&&... args)
        {
            mObjects.push_back(makeUnique<T>(params, std::forward<Args>(args)...));
            return dynamic_cast<T*>(mObjects.back().get());
        }

        [[nodiscard]] const std::vector<UPtr<Object>>& getObjects() const noexcept
        {
            return mObjects;
        }

    private:
        std::string                 mName;
        std::vector<UPtr<Object>>   mObjects;
    };
}
