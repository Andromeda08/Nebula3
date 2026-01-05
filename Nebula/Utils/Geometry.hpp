#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace geom
{
	struct Data {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		// uvs
		std::vector<uint32_t> indices;

		void setPosition(const glm::vec3& pos) {
			for (auto& e : vertices) {
				e += pos;
			}
		}
	};

	// From ECG solution
	Data createSphereData(float radius, uint32_t latSeg, uint32_t lngSeg) {
		Data data;

		// Center
		data.vertices.emplace_back(0.f, radius, 0.f);
		data.vertices.emplace_back(0.f, -radius, 0.f);
		data.normals.emplace_back(0.f, 1.f, 0.f);
		data.normals.emplace_back(0.f, -1.f, 0.f);

		// First/Last rings
		for (auto j = 0; j < lngSeg; j++) {
			data.indices.push_back(0);
			data.indices.push_back(j == lngSeg - 1 ? 2 : (j + 3));
			data.indices.push_back(2 + j);

			data.indices.push_back(2 + (latSeg - 2) * lngSeg + j);
			data.indices.push_back(j == lngSeg - 1 ? 2 + (latSeg - 2) * lngSeg : 2 + (latSeg - 2) * lngSeg + j + 1);
			data.indices.push_back(1);
		}

		// Rest of rings
		for (auto i = 1; i < latSeg; i++) {
			float angleV = static_cast<float>(i) * glm::pi<float>() / static_cast<float>(latSeg);
			for (auto j = 0; j < lngSeg; j++) {
				float angleH = static_cast<float>(j) * glm::two_pi<float>() / static_cast<float>(lngSeg);
				glm::vec3 position = {
					radius * glm::sin(angleV) * glm::cos(angleH),
					radius * glm::cos(angleV),
					radius * glm::sin(angleV) * glm::sin(angleH)
				};
				data.vertices.push_back(position);
				data.normals.push_back(glm::normalize(position));

				if (i == 1) continue;

				data.indices.push_back(2 + (i - 1) * lngSeg + j);
				data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 2) * lngSeg : 2 + (i - 2) * lngSeg + j + 1);
				data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 1) * lngSeg : 2 + (i - 1) * lngSeg + j + 1);

				data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 2) * lngSeg : 2 + (i - 2) * lngSeg + j + 1);
				data.indices.push_back(2 + (i - 1) * lngSeg + j);
				data.indices.push_back(2 + (i - 2) * lngSeg + j);
			}
		}

		return data;
	}

	// From ECG solution
	Data createCylinderData(uint32_t seg, float radius, float height) {
		Data data;

		// Center
		data.vertices.emplace_back(0.f, -height / 2.f, 0.f);
		data.vertices.emplace_back(0.f, height / 2.f, 0.f);
		data.normals.emplace_back(0.f, -1.f, 0.f);
		data.normals.emplace_back(0.f, 1.f, 0.f);

		// Circle
		float step = glm::two_pi<float>() / static_cast<float>(seg);
		for (auto i = 0; i < seg; i++) {
			glm::vec3 position = {
				glm::cos(i * step) * radius,
				-height / 2.f,
				glm::sin(i * step) * radius
			};

			// Bottom ring
			data.vertices.push_back(position);
			data.vertices.push_back(position);
			data.normals.emplace_back(0.f, -1.f, 0.f);
			data.normals.push_back(glm::normalize(position - glm::vec3(0, -height / 2.f, 0)));

			// Top ring
			position.y = height / 2.f;
			data.vertices.push_back(position);
			data.vertices.push_back(position);
			data.normals.emplace_back(0.f, 1.f, 0.f);
			data.normals.push_back(glm::normalize(position - glm::vec3(0, height / 2.f, 0)));

			// Bottom face
			data.indices.push_back(0);
			data.indices.push_back(2 + i * 4);
			data.indices.push_back(i == seg - 1 ? 2 : 2 + (i + 1) * 4);

			// Top face
			data.indices.push_back(1);
			data.indices.push_back(i == seg - 1 ? 4 : (i + 2) * 4);
			data.indices.push_back((i + 1) * 4);

			// Side faces
			data.indices.push_back(3 + i * 4);
			data.indices.push_back(i == seg - 1 ? 5 : 5 + (i + 1) * 4);
			data.indices.push_back(i == seg - 1 ? 3 : 3 + (i + 1) * 4);

			data.indices.push_back(i == seg - 1 ? 5 : 5 + (i + 1) * 4);
			data.indices.push_back(3 + i * 4);
			data.indices.push_back(5 + i * 4);
		}

		return data;
	}

}