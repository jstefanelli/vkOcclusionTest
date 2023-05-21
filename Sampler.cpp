//
// Created by barba on 16/05/2023.
//

#include "Sampler.h"

Sampler::Sampler(std::shared_ptr<Instance> inst, vk::Filter minFilter, vk::Filter magFilter, vk::SamplerMipmapMode mipMode) : instance(std::move(inst)) {
	vk::SamplerCreateInfo info({}, magFilter, minFilter, mipMode, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge);
	info.unnormalizedCoordinates = false;
	info.compareEnable = false;

	_sampler = instance->device().createSampler(info);
}

Sampler::~Sampler() {
	if (_sampler) {
		instance->device().destroySampler(_sampler);
	}
}
