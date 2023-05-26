#include "Scene.h"
#include <stdexcept>
#include <unordered_map>

#define MAKE_BATCH_ID(matId, meshId) ((static_cast<uint64_t>(matId) << 32) + meshId)

Scene::Scene(std::shared_ptr<Instance> inst, size_t maxVertexAmount, size_t maxObjectAmount) : instance(std::move(inst)), _maxObjects(maxObjectAmount), _nextObjectId(0) {
	_meshes = std::make_unique<MeshBuffer>(instance, maxVertexAmount);
}

void Scene::fill_buffers(const std::unique_ptr<Buffer> &instanceBuffer, const std::unique_ptr<Buffer> &batchBuffer,
						 const std::unique_ptr<Buffer> &drawBuffer, const std::unique_ptr<Buffer> &clearBuffer) {
	if (!_instancesUpToDate) {
		sort_instances();
	}

	if (batchBuffer != nullptr)
	{
		auto mapping = batchBuffer->map_t<DrawBatch>();
		mapping.fill_from(_batches);
	}

	if (instanceBuffer != nullptr)
	{
		auto mapping = instanceBuffer->map_t<ObjectInstance>();
		mapping.fill_from(_instances);
	}

	if (drawBuffer != nullptr)
	{
		auto mapping = drawBuffer->map_t<VkDrawIndirectCommand>();
		if (mapping.size() < _batches.size()) {
			throw std::runtime_error("Not enough space for all draw batches in drawBuffer");
		}

		uint32_t total = 0;
		for(int i = 0; i < _batches.size(); i++) {
			const auto& batch = _batches[i];
			const auto& mesh = _meshes->meshes()[batch.meshId];

			mapping[i].firstInstance = total;
			mapping[i].instanceCount = batch.amount;
			mapping[i].firstVertex = mesh.vertexOffset;
			mapping[i].vertexCount = mesh.vertexAmount;

			total += batch.amount;
		}
	}

	if (clearBuffer != nullptr) {
		auto mapping = clearBuffer->map_t<VkDrawIndirectCommand>();
		if (mapping.size() < _batches.size()) {
			throw std::runtime_error("Not enough space for all draw batches in drawBuffer");
		}

		uint32_t total = 0;
		for(int i = 0; i < _batches.size(); i++) {
			const auto& batch = _batches[i];
			const auto& mesh = _meshes->meshes()[batch.meshId];

			mapping[i].firstInstance = total;
			mapping[i].instanceCount = 0;
			mapping[i].firstVertex = mesh.vertexOffset;
			mapping[i].vertexCount = mesh.vertexAmount;

			total += batch.amount;
		}
	}
}

void Scene::sort_instances() {
	std::sort(_batches.begin(), _batches.end(), [](const DrawBatch& left, const DrawBatch& right) {
		return left.materialId < right.materialId || left.meshId < right.meshId;
	});

	std::unordered_map<uint64_t, uint32_t> positions;
	std::unordered_map<uint64_t, uint32_t> ids;

	uint32_t total = 0;
	uint32_t id = 0;
	for(auto& b : _batches) {
		auto batchId = MAKE_BATCH_ID(b.materialId, b.meshId);
		positions[batchId] = total;
		ids[batchId] = id++;

		total += b.amount;
	}

	_instances.clear();
	_instances.resize(_objects.size());
	for(auto& obj : _objects) {
		auto batchId = MAKE_BATCH_ID(obj.materialId, obj.meshId);
		auto idx = positions[batchId];
		auto batchIndex = ids[batchId];

		_instances[idx].model = obj.transform.model();
		_instances[idx].materialMeshBatchId = glm::ivec4(obj.materialId, obj.meshId, batchIndex, 0);
		_instances[idx].bbCenter = glm::vec4(_meshes->meshes()[obj.meshId].bbCenter, 1.0);
		_instances[idx].bbSize = glm::vec4(_meshes->meshes()[obj.meshId].bbExtents, 0.0);

		positions[batchId] = idx + 1;
	}

	_instancesUpToDate = true;
}

uint32_t Scene::addObject(uint32_t meshId, uint32_t materialId) {
	Object obj(_nextObjectId++, meshId, materialId);

	bool batchFound = false;
	for(auto it = _batches.begin(); it != _batches.end(); it++) {
		if (it->meshId == obj.meshId && it->materialId == obj.materialId) {
			it->amount++;
			batchFound = true;
			break;
		}
	}

	if (!batchFound) {
		_batches.emplace_back(meshId, materialId, 1, 0);
	}
	_instancesUpToDate = false;

	_objects.push_back(obj);
	return obj.objectId;
}

Object& Scene::get_object(uint32_t id) {
	auto it = std::find_if(_objects.begin(), _objects.end(), [id](const Object& obj) {
		return obj.objectId == id;
	});

	if (it == _objects.end()) {
		throw std::runtime_error("Could not find object");
	}

	return *it;
}

bool Scene::remove_object(uint32_t id) {
	auto it = std::find_if(_objects.begin(), _objects.end(), [id](const Object& obj) {
		return obj.objectId == id;
	});

	if (it != _objects.end()) {
		_objects.erase(it);
		_instancesUpToDate = false;
		return true;
	}
	return false;
}
