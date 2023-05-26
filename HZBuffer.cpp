#include "HZBuffer.h"
#include <stdexcept>

HZBuffer::HZBuffer(std::shared_ptr<Instance> inst, glm::ivec2 size, const vk::DescriptorSetLayout& downsampleLayout, const vk::Sampler& downsampleSampler) :
	instance(std::move(inst)), _texture(instance, vk::Format::eR32Sfloat, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage, size),
	_depth_texture(instance, vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, size)
{
	_sizes.push_back(size);
	//Calculate MIP sizes
	while(size.x > 1 || size.y > 1) {
		size.x = glm::max(size.x / 2, 1);
		size.y = glm::max(size.y / 2, 1);
		_sizes.push_back(size);
	}

	if (_sizes.size() != _texture.levels()) {
		throw std::runtime_error("Texture/HZB level count mismatch");
	}

	//Create Color Image views
	vk::ComponentMapping mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
								 vk::ComponentSwizzle::eA);
	for(int level = 0; level < _texture.levels(); level++) {

		vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, level, 1, 0, 1);

		vk::ImageViewCreateInfo info({}, _texture.image(), vk::ImageViewType::e2D, _texture.format(), mapping, range);
		_level_views.push_back(instance->device().createImageView(info));
	}

	//Create Depth View
	vk::ImageViewCreateInfo depthViewInfo({}, _depth_texture.image(), vk::ImageViewType::e2D, _depth_texture.format(), mapping, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
	_depth_view = instance->device().createImageView(depthViewInfo);

	vk::ImageViewCreateInfo fullViewInfo({}, _texture.image(), vk::ImageViewType::e2D, _texture.format(), mapping, {vk::ImageAspectFlagBits::eColor, 0, _texture.levels(), 0, 1});
	_full_view = instance->device().createImageView(fullViewInfo);

	//Create descriptor sets
	std::vector<vk::DescriptorSetLayout> layouts(_texture.levels() - 1, downsampleLayout);

	_downsample_descriptor_sets = instance->create_descriptor_sets(layouts);

	//Fill descriptor sets
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(2 * (_texture.levels() - 1));
	std::vector<vk::WriteDescriptorSet> writeOperations;
	writeOperations.reserve(2 * (_texture.levels() - 1));

	for(int level = 1; level < _texture.levels(); level++) {
		imageInfos.emplace_back(nullptr, _level_views[level - 1], vk::ImageLayout::eGeneral);
		imageInfos.emplace_back(nullptr, _level_views[level], vk::ImageLayout::eGeneral);

		auto* ptr0 = &imageInfos[(level - 1) * 2];
		auto* ptr1 = &imageInfos[((level - 1) * 2) + 1];
		vk::WriteDescriptorSet write0(_downsample_descriptor_sets[level - 1], 0, 0, 1, vk::DescriptorType::eStorageImage, ptr0); //I did not realize the last parameter took a pointer
		vk::WriteDescriptorSet write1(_downsample_descriptor_sets[level - 1], 1, 0, 1, vk::DescriptorType::eStorageImage, ptr1); //and not a copy....

		writeOperations.push_back(write0);
		writeOperations.push_back(write1);
	}

	instance->device().updateDescriptorSets(writeOperations, nullptr);
	instance->device().waitIdle();
}

HZBuffer::~HZBuffer() {
	for(auto& view : _level_views) {
		instance->device().destroyImageView(view);
	}

	instance->device().destroyImageView(_depth_view);
	instance->device().destroyImageView(_full_view);

	_level_views.clear();
	_sizes.clear();
}
