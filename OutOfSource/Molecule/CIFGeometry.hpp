#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Core/Types.hpp"
#include "Math/Transform.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "Scene/Types/GPUObjectInstanceData.hpp"

namespace geo
{
	struct BondTransform {
		glm::vec3 position;
		glm::vec3 axis;
		float angle;
		float dist;
	};

	struct Data {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		// uvs
		std::vector<uint32_t> indices;

		void setPosition(const glm::vec3& pos);

		void setRotation(const glm::vec3& axis, float angle);
	};

	BondTransform calcBondTransforms(const glm::vec3& atom1, const glm::vec3& atom2);

	// From ECG solution
	Data createSphereData(float radius, uint32_t latSeg, uint32_t lngSeg);

	Data createSphereData2(float r, int32_t lat, int32_t lng);

	// From ECG solution
	Data createCylinderData(uint32_t seg, float radius, float height);

	struct CIFInstanceData {
		UPtr<Geometry> sphere = makeUnique<Sphere>(Sphere::Params { 1.0f, 20, 20 });
		UPtr<Geometry> cylinder = makeUnique<Cylinder>(Cylinder::Params { .05f, 1.f, 8 });

		std::vector<GPUObjectInstanceData> sphereTransforms;
		std::vector<GPUObjectInstanceData> cylinderTransforms;
	};
}
