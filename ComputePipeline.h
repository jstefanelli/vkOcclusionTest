#ifndef VKOCCLUSIONTEST_COMPUTEPIPELINE_H
#define VKOCCLUSIONTEST_COMPUTEPIPELINE_H

#include <vulkan/vulkan.hpp>
#include <memory>
#include <filesystem>
#include "Instance.h"

class ComputePipeline {
private:
	std::shared_ptr<Instance> instance;
	std::vector<vk::ShaderModule> _modules;
	vk::Pipeline _pipeline;
	vk::DescriptorSetLayout _descriptor_layout;
	vk::PipelineLayout _pipeline_layout;
public:
	ComputePipeline(std::shared_ptr<Instance> inst, const std::filesystem::path& shaderPath, const std::vector<vk::DescriptorSetLayoutBinding>& descriptorBindings);
	~ComputePipeline();

	inline const vk::Pipeline& pipeline() const {
		return _pipeline;
	}

	inline operator const vk::Pipeline&() const {
		return _pipeline;
	}

	inline const vk::DescriptorSetLayout& descriptor_set_layout() const {
		return _descriptor_layout;
	}

	inline const vk::PipelineLayout& pipeline_layout() const {
		return _pipeline_layout;
	}
};

#endif //VKOCCLUSIONTEST_COMPUTEPIPELINE_H
