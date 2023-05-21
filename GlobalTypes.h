#ifndef VKOCCLUSIONTEST_GLOBALTYPES_H
#define VKOCCLUSIONTEST_GLOBALTYPES_H

#include <glm/glm.hpp>

typedef struct Vertex_Data {
	alignas(16)	glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 tangent;
	alignas(16) glm::vec4 uv;
	glm::vec4 boneWeights;
	glm::ivec4 boneIds;
	glm::vec4 vertexColor;

	inline explicit Vertex_Data(glm::vec3 position = {}, glm::vec3 normal = {}, glm::vec3 tangent = {}, glm::vec4 uv = {}, glm::vec4 boneWeights = {}, glm::ivec4 boneIds = {}, glm::vec4 vertexColor = {})
			: position(position), normal(normal), tangent(tangent), uv(uv), boneWeights(boneWeights), boneIds(boneIds), vertexColor(vertexColor) {

	}
} Vertex_Data;


typedef struct Push_Constants {
	alignas(16) glm::mat4 view;
	glm::mat4 projection;

	inline explicit Push_Constants(glm::mat4 view, glm::mat4 projection) : view(view), projection(projection) {

	}
} Push_Constants;

typedef struct {
	alignas(16) glm::mat4 modelMatrix;
} Instance_Data;

#endif //VKOCCLUSIONTEST_GLOBALTYPES_H
