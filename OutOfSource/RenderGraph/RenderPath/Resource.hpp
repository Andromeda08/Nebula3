#pragma once

#include <string>
#include <type_traits>

#include "RenderGraph/RenderGraphTraits.hpp"

namespace rg
{
    // Render Graph resource base class.
    class Resource
    {
    public:
        Resource(std::string name, ResourceType resourceType);

        virtual ~Resource() = default;

        template <class T>
        T* as()
        {
            static_assert(std::is_base_of_v<Resource, T>, "Template parameter T must be a valid Resource type");
            return dynamic_cast<T*>(this);
        }

        [[nodiscard]] const std::string& getName() const noexcept;

        ResourceType getResourceType() const noexcept;

    private:
        std::string     mName;
        ResourceType    mResourceType;
    };
}
