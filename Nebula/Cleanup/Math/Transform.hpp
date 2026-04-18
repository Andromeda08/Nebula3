#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "UserInterface/IComponent.hpp"

namespace vk
{
    struct TransformMatrixKHR;
}

namespace nbl
{
    enum class TransformType : int32_t
    {
        EulerAngle  = 0,
        AxisAngle   = 1,
        Quaternion  = 2,
        ModelMatrix = 3,
    };

    [[nodiscard]] constexpr std::string xformTypeToString(const TransformType type) noexcept
    {
        using enum TransformType;
        switch (type)
        {
            case EulerAngle:  return "Euler";
            case AxisAngle:   return "Axis-angle";
            case Quaternion:  return "Quat";
            case ModelMatrix: return "Model Matrix (const)";
        }
        std::unreachable();
    }

    /**
     * Transform class with Translate, Scale and Rotation.
     * - Rotation mode can be chosen via "setMode", defaults to Euler angles.
     * - Transform matrix calculation is delayed until getModel() is called.
     * - "TransformEditorComponent" UI component can be used edit transforms during runtime.
     */
    class Transform
    {
    public:
        Transform() = default;

        // Create constant transform from model matrix
        explicit Transform(const glm::mat4& matrix);

        Transform& setMode(TransformType mode) noexcept;

        [[nodiscard]] bool isDirty() const noexcept;

        // Translate
        // ============================

        Transform& translate(const glm::vec3& val) noexcept;

        Transform& setTranslate(const glm::vec3& val) noexcept;

        // Scale
        // ============================

        Transform& scale(const glm::vec3& val) noexcept;

        Transform& setScale(const glm::vec3& val) noexcept;

        // Euler Angles
        // ============================

        Transform& rotateEuler(const glm::vec3& val) noexcept;

        Transform& setRotationEuler(const glm::vec3& val) noexcept;

        // Angle-axis
        // ============================
        Transform& setRotationAngleAxis(const glm::vec3& ax, float a) noexcept;

        Transform& rotateAngle(float a) noexcept;

        Transform& setRotationAngle(float a) noexcept;

        Transform& rotateAxis(const glm::vec3& dAx) noexcept;

        Transform& setRotationAxis(const glm::vec3& ax) noexcept;

        // Quaternion
        // ============================
        Transform& setRotationQuat(const glm::quat& q) noexcept;

        // Matrix
        // ============================

        [[nodiscard]] const glm::mat4& getModel() noexcept;

        [[nodiscard]] vk::TransformMatrixKHR getModel3x4() noexcept;

    private:
        void wrapRotationAngles() noexcept;

        [[nodiscard]] static float wrapAngle(float a) noexcept;

        friend class TransformEditorComponent;

        glm::vec3       mTranslate = glm::vec3(0.0f);
        glm::vec3       mScale     = glm::vec3(1.0f);

        // EulerAngle
        glm::vec3       mEuler     = glm::vec3(0.0f);

        // AxisAngle
        glm::vec3       mAxis      = glm::vec3(0.0f, 1.0f, 0.0f);
        float           mAngle     = 0.0f;

        // Quaternion
        glm::quat       mQuat      = glm::quat(1, 0, 0, 0);

        TransformType   mMode      = TransformType::EulerAngle;
        bool            mDirty     = false;
        bool            mConst     = false;
        glm::mat4       mMatrix    = glm::mat4(1.0f);
    };

    class TransformEditorComponent : public IComponent
    {
    public:
        explicit TransformEditorComponent(Transform* pTransform);

        void draw() override;

    private:
        std::vector<std::string> mModes;
        Transform*               mTransform = nullptr;
    };
}
