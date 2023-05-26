#include "ComputePipeline.h"
#include "Utils.h"
#include <stdexcept>

ComputePipeline::ComputePipeline(std::shared_ptr<Instance> inst, const std::filesystem::path &shaderPath,
								 const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &descriptorBindings,
								 const std::vector<vk::PushConstantRange>& pushConstants) : instance(std::move(inst)) {
	auto mod = createShaderModule(shaderPath, instance->device());

	_modules.push_back(mod);

	for(auto& b : descriptorBindings) {
		auto _descriptor_layout = instance->device().createDescriptorSetLayout({{}, b});
		_descriptor_layouts.push_back(_descriptor_layout);
	}

	_pipeline_layout = instance->device().createPipelineLayout({{}, _descriptor_layouts, pushConstants });

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

	for(auto& l : _descriptor_layouts) {
		instance->device().destroyDescriptorSetLayout(l);
	}
	_descriptor_layouts.clear();

	for(auto& mod : _modules) {
		instance->device().destroyShaderModule(mod);
	}
	_modules.clear();
}
