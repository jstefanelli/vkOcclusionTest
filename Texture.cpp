#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include "Buffer.h"

Texture::Texture(std::shared_ptr<Instance> inst, vk::Format format, vk::ImageUsageFlags flags, glm::ivec2 sz, uint32_t levels) : instance(std::move(inst)), _format(format), _size(sz), _flags(flags) {

	if (levels == TEXTURE_LEVELS_AUTO) {
		int width = _size.x, height = _size.y;
		levels = 1;
		while(width > 1 || height > 1) {
			width = glm::max(width / 2, 1);
			height = glm::max(height / 2, 1);
			levels++;
		}
	}
	_levels = levels;

	auto queue_families = std::array<uint32_t, 2>{ instance->graphics_queue_index(), instance->compute_queue_index() };
	vk::ImageCreateInfo imageInfo( {}, vk::ImageType::e2D, _format, { (uint32_t) _size.x, (uint32_t) _size.y, 1 }, _levels, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, flags, vk::SharingMode::eExclusive, queue_families);

	_image = instance->device().createImage(imageInfo);

	auto memoryRequirements = instance->device().getImageMemoryRequirements(_image);

	auto any = false;
	auto memoryIndex = 0;
	auto index = 0;
	for(auto& t : instance->memory_properties().memoryTypes) {
		if ((memoryRequirements.memoryTypeBits & (1 << index)) == 0) {
			continue;
		}

		if ((t.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal &&
			(instance->memory_properties().memoryHeaps[t.heapIndex].flags & vk::MemoryHeapFlagBits::eDeviceLocal) == vk::MemoryHeapFlagBits::eDeviceLocal) {
			memoryIndex = index;
			any = true;
			break;
		}

		index++;
	}

	if(!any) {
		throw std::runtime_error("Failed to find adequate memory type for texture");
	}

	vk::MemoryAllocateInfo memoryInfo(memoryRequirements.size, memoryIndex);
	_memory = instance->device().allocateMemory(memoryInfo);

	instance->device().bindImageMemory(_image, _memory, 0);
}

Texture::~Texture() {
	instance->device().destroyImage(_image);
	instance->device().freeMemory(_memory);
}

bool Texture::fill_from_file(const vk::CommandBuffer& cmd, const std::filesystem::path &path, uint32_t level, vk::ImageLayout targetLayout) {
	auto targetWidth = _size.x, targetHeight = _size.y;
	for(int i = 0; i < level; i++) {
		targetWidth = glm::max(targetWidth / 2, 1);
		targetHeight = glm::max(targetHeight / 2, 1);
	}

	int fileWidth = 0, fileHeight = 0, fileChannels = 0;

	stbi_uc* pixels = stbi_load(path.string().c_str(), &fileWidth, &fileHeight, &fileChannels, STBI_rgb_alpha);

	if (pixels == nullptr) {
		return false;
	}

	auto tmpBuffer = Buffer(instance, fileWidth * fileHeight * fileChannels, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	{
		auto map = tmpBuffer.map();
		std::memcpy(static_cast<void*>(map), (void*)pixels, tmpBuffer.size());
	}

	stbi_image_free(pixels);

	vk::ImageSubresourceRange subResourceRange( vk::ImageAspectFlagBits::eColor, level, 1, 0, 1);
	vk::ImageMemoryBarrier transfer_barrier(vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {}, _image, subResourceRange);

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, transfer_barrier);

	vk::BufferImageCopy imageCopy(0, 0, 0, { vk::ImageAspectFlagBits::eColor, level, 0, 1}, { fileWidth, fileHeight, 1});
	cmd.copyBufferToImage(static_cast<vk::Buffer>(tmpBuffer), _image, vk::ImageLayout::eTransferDstOptimal, imageCopy);

	vk::ImageMemoryBarrier readable_barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal, targetLayout, {}, {}, _image, subResourceRange);
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, readable_barrier);

	return true;
}

bool Texture::fill_from_data(const vk::CommandBuffer &cmd, const std::vector<uint8_t> &data, uint32_t level,
							 uint32_t channels, vk::ImageLayout targetLayout) {
	level = glm::max(level, 1U);

	auto targetWidth = _size.x, targetHeight = _size.y;
	for(int i = 0; i < level; i++) {
		targetWidth = glm::max(targetWidth / 2, 1);
		targetHeight = glm::max(targetHeight / 2, 1);
	}

	if (targetWidth * targetHeight * channels > data.size()) {
		return false;
	}

	auto buffer = Buffer(instance, targetWidth * targetHeight * channels, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	{
		auto map = buffer.map();
		std::memcpy(static_cast<void*>(map), data.data(), buffer.size());
	}

	vk::ImageSubresourceRange subResourceRange( vk::ImageAspectFlagBits::eColor, level, 1, 0, 1);
	vk::ImageMemoryBarrier transfer_barrier(vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {}, _image, subResourceRange);

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, transfer_barrier);

	vk::BufferImageCopy imageCopy(0, 0, 0, { vk::ImageAspectFlagBits::eColor, level, 0, 1}, { targetWidth, targetHeight, 1});
	cmd.copyBufferToImage(static_cast<vk::Buffer>(buffer), _image, vk::ImageLayout::eTransferDstOptimal, imageCopy);

	vk::ImageMemoryBarrier readable_barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal, targetLayout, {}, {}, _image, subResourceRange);
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, readable_barrier);
	return true;
}
