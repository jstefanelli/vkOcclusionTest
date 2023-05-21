#include "ComputePipeline.h"
#include "Utils.h"
#include <stdexcept>

ComputePipeline::ComputePipeline(std::shared_ptr<Instance> inst, const std::filesystem::path &shaderPath,
								 const std::vector<vk::DescriptorSetLayoutBinding> &descriptorBindings) : instance(std::move(inst)) {
	auto mod = createShaderModule(shaderPath, instance->device());

	_modules.push_back(mod);

	_descriptor_layout = instance->device().createDescriptorSetLayout({{}, descriptorBindings});
	_pipeline_layout = instance->device().createPipelineLayout({{}, _descriptor_layout});

	vk::PipelineShaderStageCreateInfo shaderStage( {}, vk::ShaderStageFlagBits::eCompute, mod, "main");

	vk::ComputePipelineCreateInfo info( {}, shaderStage, _pipeline_layout );
	auto res = instance->device().createComputePipeline(nullptr, info);
	switch(res.result) {
		case vk::Result::eSuccess:
			_pipeline = res.value;
			return;
		default:
			throw std::runtime_error("Failed to create compute pipeline");
	}
}

ComputePipeline::~ComputePipeline() {
	if (_pipeline) {
		instance->device().destroyPipeline(_pipeline);
	}

	if (_pipeline_layout) {
		instance->device().destroyPipelineLayout(_pipeline_layout);
	}

	if (_descriptor_layout) {
		instance->device().destroyDescriptorSetLayout(_descriptor_layout);
	}

	for(auto& mod : _modules) {
		instance->device().destroyShaderModule(mod);
	}
	_modules.clear();
}
