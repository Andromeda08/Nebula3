#include "Transform.hpp"

#include <vulkan/vulkan.hpp>

vk::TransformMatrixKHR Transform::getModel3x4() noexcept
{
    const auto m = getModel();
    return vk::TransformMatrixKHR({
        std::array { m[0].x, m[1].x, m[2].x, m[3].x },
        std::array { m[0].y, m[1].y, m[2].y, m[3].y },
        std::array { m[0].z, m[1].z, m[2].z, m[3].z },
    });
}
