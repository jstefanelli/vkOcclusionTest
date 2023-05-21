#ifndef VKOCCLUSIONTEST_BUFFER_H
#define VKOCCLUSIONTEST_BUFFER_H

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "Instance.h"

class BufferMapping;

class Buffer {
	friend BufferMapping;
private:
	std::shared_ptr<Instance> instance;
	vk::Buffer buffer;
	vk::DeviceMemory bufferMemory;
	size_t bufferSize;
public:
	static uint32_t findMemoryType(const vk::ArrayProxy<vk::MemoryType>& types, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	Buffer(std::shared_ptr<Instance> instance, size_t bufferSize, vk::BufferUsageFlags bufferFlags, vk::MemoryPropertyFlags memoryFlags);
	void copy_to(const Buffer& other, vk::CommandBuffer& commandBuffer);
	inline explicit operator vk::Buffer() const {
		return buffer;
	}

	inline size_t size() const {
		return bufferSize;
	}

	BufferMapping map() const;

	void clean();
	~Buffer();
};


class BufferMapping {
	friend Buffer;
private:
	const Buffer& buffer;
	const vk::Device& device;
	void* data;
	BufferMapping(const vk::Device& device, const Buffer& buffer);
public:
	BufferMapping(const BufferMapping&) = delete;
	BufferMapping(const BufferMapping&&) = delete;

	inline explicit operator void*() const {
		return data;
	}

	~BufferMapping();
};

#endif //VKOCCLUSIONTEST_BUFFER_H
