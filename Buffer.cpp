//
// Created by barba on 07/05/2023.
//

#include "Buffer.h"

uint32_t Buffer::findMemoryType(const vk::ArrayProxy<vk::MemoryType> &types, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < types.size(); i++) {
		if ((typeFilter & (1 << i)) && (types.data()[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Could not find memory property");
}

Buffer::Buffer(std::shared_ptr<Instance> _instance, size_t bufferSize, vk::BufferUsageFlags bufferFlags, vk::MemoryPropertyFlags memoryFlags) : instance(std::move(_instance)) {
	auto device = instance->device();

	_buffer = device.createBuffer({{}, bufferSize, bufferFlags, vk::SharingMode::eExclusive });
	auto requirements = device.getBufferMemoryRequirements(_buffer);
	auto idx = findMemoryType(instance->memory_properties().memoryTypes, requirements.memoryTypeBits, memoryFlags);

	bufferMemory = device.allocateMemory({ requirements.size, idx });
	device.bindBufferMemory(_buffer, bufferMemory, 0);
	this->bufferSize = bufferSize;
}

void Buffer::clean() {
	if (!_buffer) {
		return;
	}

	auto device = instance->device();
	device.destroyBuffer(_buffer);
	device.freeMemory(bufferMemory);
}

Buffer::~Buffer() {
	clean();
}

BufferMapping Buffer::map() const {
	return {instance->device(), *this};
}

void Buffer::copy_to(const Buffer &other, const vk::CommandBuffer& commandBuffer) {
	//commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}});
	commandBuffer.copyBuffer(_buffer, other._buffer, vk::BufferCopy{0, 0, bufferSize });
	//commandBuffer.end();
}

BufferMapping::BufferMapping(const vk::Device &device, const Buffer &buffer) : device(device), buffer(buffer) {
	data = device.mapMemory(buffer.memory(), 0, buffer.size());
}

BufferMapping::~BufferMapping() {
	device.unmapMemory(buffer.memory());
}
