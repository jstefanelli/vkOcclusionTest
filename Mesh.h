#ifndef VKOCCLUSIONTEST_MESH_H
#define VKOCCLUSIONTEST_MESH_H

#include <glm/glm.hpp>
#include "GlobalTypes.h"
#include "Buffer.h"
#include <vector>
#include <memory>

struct Mesh {
public:
	size_t meshId;
	int vertexAmount;
	int vertexOffset;
	int meshType;
	glm::vec3 bbCenter;
	glm::vec3 bbExtents;
};

class MeshBuffer {
private:
	std::shared_ptr<Instance> instance;
	size_t _maxVertexCount;
	size_t _nextFreeSlot;
	std::unique_ptr<Buffer> _buffer;
	std::vector<Mesh> _meshes;
public:
	MeshBuffer(std::shared_ptr<Instance> instance, size_t maxVertexCount);
	size_t append(const std::vector<Vertex>& vertices, int meshType, glm::vec3 boundingBoxCenter, glm::vec3 boundingBoxExtents, const vk::CommandBuffer& cmdBuffer);

	inline const std::vector<Mesh>& meshes() const {
		return _meshes;
	}

	inline const std::unique_ptr<Buffer>& buffer() const {
		return _buffer;
	}
};

#endif //VKOCCLUSIONTEST_MESH_H
