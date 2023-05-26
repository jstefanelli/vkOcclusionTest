#ifndef VKOCCLUSIONTEST_BUFFER_H
#define VKOCCLUSIONTEST_BUFFER_H

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "Instance.h"
#include <stdexcept>


class BufferMapping;
template<typename T>
class BufferMapping_t;

class Buffer {
private:
	std::shared_ptr<Instance> instance;
	vk::Buffer _buffer;
	vk::DeviceMemory bufferMemory;
	size_t bufferSize;
public:
	static uint32_t findMemoryType(const vk::ArrayProxy<vk::MemoryType>& types, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	Buffer(std::shared_ptr<Instance> instance, size_t bufferSize, vk::BufferUsageFlags bufferFlags, vk::MemoryPropertyFlags memoryFlags);
	void copy_to(const Buffer& other, const vk::CommandBuffer& commandBuffer);
	inline explicit operator vk::Buffer() const {
		return _buffer;
	}

	inline const vk::Buffer& buffer() const {
		return _buffer;
	}

	inline size_t size() const {
		return bufferSize;
	}

	inline const vk::DeviceMemory memory() const {
		return bufferMemory;
	}

	BufferMapping map() const;

	template<typename T>
	BufferMapping_t<T> map_t() const {
		return BufferMapping_t<T>(instance->device(), *this);
	}

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
	BufferMapping(BufferMapping&&) = delete;

	inline explicit operator void*() const {
		return data;
	}

	~BufferMapping();
};


template<typename T>
class BufferMapping_t {
	friend Buffer;
private:
	const Buffer& buffer;
	const vk::Device& device;
	T* _data;
	size_t amount;
	BufferMapping_t(const vk::Device& device, const Buffer& buffer) : device(device), buffer(buffer) {
		_data = reinterpret_cast<T*>(device.mapMemory(buffer.memory(), 0, buffer.size()));
		amount = buffer.size() / sizeof(T);
	}
public:
	BufferMapping_t(const BufferMapping&) = delete;
	BufferMapping_t(BufferMapping&&) = delete;

	inline operator T*() const {
		return _data;
	}

	inline T* data() const {
		return _data;
	}

	inline size_t size() const {
		return amount;
	}

	// Copy the contents of 'objects' into 'data()'
	void fill_from(const std::vector<T>& objects) {
		if (amount < objects.size()) {
			throw std::runtime_error("Not enough space to fill buffer");
		}

		std::memcpy(reinterpret_cast<void*>(_data), objects.data(), sizeof(T) * objects.size());
	}

	~BufferMapping_t() {
		device.unmapMemory(buffer.memory());
	}
};

#endif //VKOCCLUSIONTEST_BUFFER_H
