#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "Math/Transform.hpp"

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
		Data sphere = createSphereData2(1.0f, 20, 20);
		Data cylinder = createCylinderData(8, .05f, 1.f);

		std::vector<glm::mat4> sphereTransforms;
		std::vector<glm::mat4> cylinderTransforms;
	};
}
