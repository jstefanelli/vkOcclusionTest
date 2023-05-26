#ifndef VKOCCLUSIONTEST_SCENE_H
#define VKOCCLUSIONTEST_SCENE_H

#include "Transform.h"
#include "Mesh.h"
#include "Buffer.h"
#include "GlobalTypes.h"
#include <vector>

struct Object {
	uint32_t objectId;
	uint32_t meshId;
	uint32_t materialId;
	Transform transform;
};


class Scene {
private:
	std::shared_ptr<Instance> instance;
	std::unique_ptr<MeshBuffer> _meshes;
	std::vector<Object> _objects;
	std::vector<DrawBatch> _batches;

	std::vector<ObjectInstance> _instances;

	bool _instancesUpToDate;
	size_t _maxObjects;
	uint32_t _nextObjectId;

	void sort_instances();
public:
	Scene(std::shared_ptr<Instance> inst, size_t maxVertexAmount, size_t maxObjectAmount);

	void fill_buffers(const std::unique_ptr<Buffer>& instanceBuffer, const std::unique_ptr<Buffer>& batchBuffer, const std::unique_ptr<Buffer>& drawBuffer, const std::unique_ptr<Buffer>& clearBuffer);

	uint32_t addObject(uint32_t meshId, uint32_t materialId);
	Object& get_object(uint32_t id);
	bool remove_object(uint32_t id);

	inline const size_t batches_amount() const {
		return _batches.size();
	}

	inline const std::vector<Object>& objects() const {
		return _objects;
	}

	inline size_t max_objects() const {
		return _maxObjects;
	}

	inline const std::unique_ptr<MeshBuffer>& meshes() const {
		return _meshes;
	}
};

#endif //VKOCCLUSIONTEST_SCENE_H
