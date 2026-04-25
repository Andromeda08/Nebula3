#include "Transform.hpp"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <vulkan/vulkan.hpp>

namespace nbl
{
    Transform::Transform(const glm::mat4& matrix)
    : mMode(TransformType::ModelMatrix)
    , mDirty(false)
    , mConst(true)
    , mMatrix(matrix)
    {
    }

    Transform& Transform::setMode(const TransformType mode) noexcept
    {
        mMode = mode;
        return *this;
    }

    bool Transform::isDirty() const noexcept
    {
        return mDirty;
    }

    #pragma region "Transform modifiers"

    Transform& Transform::translate(const glm::vec3& val) noexcept
    {
        mTranslate += val;
        mDirty     = true;
        return *this;
    }

    Transform& Transform::setTranslate(const glm::vec3& val) noexcept
    {
        mTranslate = val;
        mDirty     = true;
        return *this;
    }

    Transform& Transform::scale(const glm::vec3& val) noexcept
    {
        mScale += val;
        mDirty = true;
        return *this;
    }

    Transform& Transform::setScale(const glm::vec3& val) noexcept
    {
        mScale = val;
        mDirty = true;
        return *this;
    }

    Transform& Transform::rotateEuler(const glm::vec3& val) noexcept
    {
        mEuler += val;
        wrapRotationAngles();
        mDirty = true;
        return *this;
    }

    Transform& Transform::setRotationEuler(const glm::vec3& val) noexcept
    {
        mEuler = val;
        wrapRotationAngles();
        mDirty = true;
        return *this;
    }

    Transform& Transform::setRotationAngleAxis(const glm::vec3& ax, const float a) noexcept
    {
        mAxis  = ax;
        mAngle = wrapAngle(a);
        mDirty = true;
        return *this;
    }

    Transform& Transform::rotateAngle(const float a) noexcept
    {
        mAngle = wrapAngle(mAngle + a);
        mDirty = true;
        return *this;
    }

    Transform& Transform::setRotationAngle(const float a) noexcept
    {
        mAngle = wrapAngle(a);
        mDirty = true;
        return *this;
    }

    Transform& Transform::rotateAxis(const glm::vec3& dAx) noexcept
    {
        mAxis  += dAx;
        mDirty = true;
        return *this;
    }

    Transform& Transform::setRotationAxis(const glm::vec3& ax) noexcept
    {
        mAxis  = ax;
        mDirty = true;
        return *this;
    }

    Transform& Transform::setRotationQuat(const glm::quat& q) noexcept
    {
        mQuat  = q;
        mDirty = true;
        return *this;
    }

    #pragma endregion

    const glm::mat4& Transform::getModel() noexcept
    {
        if (mConst || mMode == TransformType::ModelMatrix)
        {
            return mMatrix;
        }

        const glm::mat4 T = glm::translate(glm::mat4(1.0f), mTranslate);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), mScale);

        auto R = glm::mat4(1.0f);
        switch (mMode)
        {
            case TransformType::EulerAngle: {
                R = glm::yawPitchRoll(glm::radians(mEuler.y), glm::radians(mEuler.x), glm::radians(mEuler.z));
                break;
            }
            case TransformType::AxisAngle: {
                R = glm::rotate(glm::mat4(1.0f), glm::radians(mAngle), mAxis);
                break;
            }
            case TransformType::Quaternion: {
                R = glm::mat4_cast(mQuat);
                break;
            }
            default: {
                break;
            }
        }

        mMatrix = T * R * S;

        return mMatrix;
    }

    vk::TransformMatrixKHR Transform::getModel3x4() noexcept
    {
        const auto m = getModel();
        return vk::TransformMatrixKHR({
            std::array { m[0].x, m[1].x, m[2].x, m[3].x },
            std::array { m[0].y, m[1].y, m[2].y, m[3].y },
            std::array { m[0].z, m[1].z, m[2].z, m[3].z },
        });
    }

    void Transform::wrapRotationAngles() noexcept
    {
        mEuler.x = wrapAngle(mEuler.x);
        mEuler.y = wrapAngle(mEuler.y);
        mEuler.z = wrapAngle(mEuler.z);
    }

    float Transform::wrapAngle(float a) noexcept
    {
        a = glm::mod(a, 360.0f);
        return a < 0 ? a + 360.0f : a;
    }

    TransformEditorComponent::TransformEditorComponent(Transform* pTransform): mTransform(pTransform)
    {
        mModes = {
            xformTypeToString(TransformType::EulerAngle),
            xformTypeToString(TransformType::AxisAngle),
            xformTypeToString(TransformType::Quaternion),
            xformTypeToString(TransformType::ModelMatrix),
        };
    }

    void TransformEditorComponent::draw()
    {
        bool&      dirty       = mTransform->mDirty;
        const auto currentMode = std::to_underlying(mTransform->mMode);

        ImGui::Begin("Transform");

        if (mTransform->mConst)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::BeginCombo("Transform Type", mModes[currentMode].c_str()))
        {
            for (int32_t i = 0; i < mModes.size(); i++)
            {
                const auto isSelected = currentMode == i;
                if (ImGui::Selectable(mModes[i].c_str(), isSelected))
                {
                    mTransform->mMode = static_cast<TransformType>(i);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (mTransform->mConst)
        {
            ImGui::EndDisabled();
        }

        const auto disableEdit = currentMode == std::to_underlying(TransformType::ModelMatrix) || mTransform->mConst;
        if (disableEdit)
        {
            ImGui::BeginDisabled();
        }

        dirty |= ImGui::DragFloat3("Translate", glm::value_ptr(mTransform->mTranslate), 0.025f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        dirty |= ImGui::DragFloat3("Scale", glm::value_ptr(mTransform->mScale), 0.025f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());

        ImGui::SeparatorText("Rotation");
        switch (mTransform->mMode)
        {
            case TransformType::EulerAngle: {
                dirty |= ImGui::DragFloat3("Rotation", glm::value_ptr(mTransform->mEuler), 0.025f, -360.0f, 360.0f, "%.3f", ImGuiSliderFlags_WrapAround);
                break;
            }
            case TransformType::AxisAngle: {
                dirty |= ImGui::DragFloat3("Axis", glm::value_ptr(mTransform->mAxis), 0.025f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
                dirty |= ImGui::DragFloat("Angle", &mTransform->mAngle, 0.025f, -360.0f, 360.0f, "%.3f", ImGuiSliderFlags_WrapAround);
                break;
            }
            case TransformType::Quaternion: {
                dirty |= ImGui::DragFloat3("Rotation", glm::value_ptr(mTransform->mEuler), 0.025f, -360.0f, 360.0f, "%.3f", ImGuiSliderFlags_WrapAround);
                break;
            }
            case TransformType::ModelMatrix: {
                break;
            }
        }

        if (disableEdit)
        {
            ImGui::EndDisabled();
        }

        ImGui::SeparatorText("Model Matrix (const)");
        const auto m = mTransform->getModel();

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame))
        {
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow();
                for (int column = 0; column < 4; column++)
                {
                    ImGui::TableSetColumnIndex(column);
                    ImGui::Text("%.3f", m[row][column]);
                }
            }
            ImGui::EndTable();
        }

        ImGui::End();
    }
}
