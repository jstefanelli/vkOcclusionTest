#ifndef VKOCCLUSIONTEST_SAMPLER_H
#define VKOCCLUSIONTEST_SAMPLER_H

#include <vulkan/vulkan.hpp>
#include <memory>
#include "Instance.h"

class Sampler {
private:
	std::shared_ptr<Instance> instance;
	vk::Sampler _sampler;
public:
	Sampler(std::shared_ptr<Instance> inst, vk::Filter minFilter, vk::Filter magFilter, vk::SamplerMipmapMode mode);
	~Sampler();

	inline const vk::Sampler& sampler() const {
		return _sampler;
	}

	inline operator const vk::Sampler&() const {
		return _sampler;
	}
};

#endif //VKOCCLUSIONTEST_SAMPLER_H
