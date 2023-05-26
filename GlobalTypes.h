#ifndef VKOCCLUSIONTEST_GLOBALTYPES_H
#define VKOCCLUSIONTEST_GLOBALTYPES_H

#include <glm/glm.hpp>
#define align_16 alignas(16)

#define v3 glm::vec3
#define v4 glm::vec4
#define m4 glm::mat4
#define i4 glm::ivec4
#define uint uint32_t

#include "shaders/libs/structures.glsl"

inline Vertex makeVertex(glm::vec3 position = {}, glm::vec3 normal = {}, glm::vec3 tangent = {}, glm::vec4 uv = {}, glm::vec4 boneWeights = {}, glm::ivec4 boneIds = {}, glm::vec4 vertexColor = {}) {
	return Vertex(position, normal, tangent, uv, boneWeights, boneIds, vertexColor);
}
inline ObjectInstance makeInstance(glm::mat4 model = glm::mat4(1.0), int materialId = 0) {
	return ObjectInstance(model, glm::ivec4(materialId));
}

#undef uint
#undef v3
#undef v4
#undef i4
#undef m4

#endif //VKOCCLUSIONTEST_GLOBALTYPES_H
