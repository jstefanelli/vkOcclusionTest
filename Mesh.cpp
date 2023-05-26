#include "Mesh.h"

MeshBuffer::MeshBuffer(std::shared_ptr<Instance> inst, size_t maxVertexCount) : instance(std::move(inst)), _maxVertexCount(maxVertexCount), _nextFreeSlot(0) {
	_buffer = std::make_unique<Buffer>(instance, sizeof(Vertex) * maxVertexCount, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
}

size_t MeshBuffer::append(const std::vector<Vertex> &vertices, int meshType, glm::vec3 boundingBoxCenter, glm::vec3 boundingBoxExtents, const vk::CommandBuffer& cmdBuffer) {
	if (_nextFreeSlot + vertices.size() >= _maxVertexCount) {
		throw std::runtime_error("Too many vertices in mesh _buffer");
	}

	auto dataSize = vertices.size() * sizeof(Vertex);
	Mesh m(_meshes.size(), vertices.size(), _nextFreeSlot, meshType, boundingBoxCenter, boundingBoxExtents);

	const auto& tmpBuffer = Instance::get_transfer_buffer(instance, dataSize);
	{
		auto map = tmpBuffer->map();
		std::memcpy(static_cast<void*>(map), vertices.data(), dataSize);
	}

	cmdBuffer.copyBuffer(tmpBuffer->buffer(), _buffer->buffer(), vk::BufferCopy(0, _nextFreeSlot * sizeof(Vertex), dataSize));
	_nextFreeSlot += vertices.size();

	_meshes.push_back(m);

	return m.meshId;
}
