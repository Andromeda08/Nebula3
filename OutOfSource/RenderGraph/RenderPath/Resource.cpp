#include "Resource.hpp"

#include <format>
#include <stdexcept>

namespace rg
{
    Resource::Resource(std::string name, const ResourceType resourceType)
    : mName(std::move(name))
    , mResourceType(resourceType)
    {
    }

    const std::string& Resource::getName() const noexcept
    {
        return mName;
    }

    ResourceType Resource::getResourceType() const noexcept
    {
        return mResourceType;
    }
}
