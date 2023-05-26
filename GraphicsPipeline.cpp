#include "GraphicsPipeline.h"
#include "Utils.h"

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Instance> _instance, const std::filesystem::path& vsPath, const std::filesystem::path& fsPath,
								   const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& bindings, const std::vector<vk::SubpassDependency>& dependencies,
								   const std::vector<GraphicsPipelineAttachment>& colorAttachmentFormats, const std::optional<GraphicsPipelineAttachment>& depthFormat)
								   : instance(std::move(_instance)) {

	auto device = instance->device();
	auto vsModule = createShaderModule(vsPath, device);
	auto fsModule = createShaderModule(fsPath, device);

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vsModule, "main" ),
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fsModule, "main")
	};

	modules.push_back(vsModule);
	modules.push_back(fsModule);

	std::vector<vk::DynamicState> dynamicState = { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eCullMode };
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicState );

	for(auto& b : bindings) {
		auto _descriptor_set_layout = device.createDescriptorSetLayout({{}, b});
		_descriptor_set_layouts.push_back(_descriptor_set_layout);
	}

	_pipeline_layout = device.createPipelineLayout({ {}, _descriptor_set_layouts });


	std::vector<vk::AttachmentDescription> colorAttachments;
	std::vector<vk::AttachmentReference> colorReferences;
	uint32_t colorAttachmentId = 0;
	for(const auto& imageFormat : colorAttachmentFormats) {
		vk::AttachmentDescription colorAttachment({}, imageFormat.format, vk::SampleCountFlagBits::e1,
												  vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
												  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
												  imageFormat.startLayout, imageFormat.endLayout);
		colorAttachments.push_back(colorAttachment);
		colorReferences.emplace_back(colorAttachmentId, vk::ImageLayout::eColorAttachmentOptimal);

		colorAttachmentId++;
	}

	vk::AttachmentReference depthReference;
	if (depthFormat.has_value()) {
		auto& val = depthFormat.value();
		vk::AttachmentDescription depthAttachment({}, val.format, vk::SampleCountFlagBits::e1,
												  vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
												  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
												  val.startLayout, val.endLayout);
		colorAttachments.push_back(depthAttachment);
		depthReference = {colorAttachmentId, vk::ImageLayout::eDepthStencilAttachmentOptimal};
	}

	vk::SubpassDescription subpassDescription( {}, vk::PipelineBindPoint::eGraphics, colorReferences, colorReferences, {}, depthFormat.has_value() ? &depthReference : nullptr);
	vk::RenderPassCreateInfo renderPassInfo {{}, colorAttachments, subpassDescription, dependencies };
	_render_pass = device.createRenderPass(renderPassInfo);

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo( {}, {}, {} );
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo( {}, vk::PrimitiveTopology::eTriangleList, false );
	vk::PipelineViewportStateCreateInfo viewportInfo( {}, 1, nullptr, 1, nullptr );

	vk::PipelineRasterizationStateCreateInfo rasterizer( {}, false, false,
														 vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
														 false, 0.0f, 0.0f, 0.0f, 1.0f );
	vk::PipelineMultisampleStateCreateInfo msInfo( {}, vk::SampleCountFlagBits::e1, false, 1.0f, nullptr, false, false );
	vk::PipelineColorBlendAttachmentState blendInfo0(false);
	blendInfo0.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo blendInfo( {}, false, vk::LogicOp::eCopy, blendInfo0, { 0.0f, 0.0f, 0.0f, 0.0f });

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo( {}, depthFormat.has_value(), depthFormat.has_value(), vk::CompareOp::eLessOrEqual, false, false);

	vk::GraphicsPipelineCreateInfo pipelineInfo( {}, shaderStages, &vertexInputInfo, &inputAssemblyInfo, nullptr,
												 &viewportInfo, &rasterizer, &msInfo, &depthStencilInfo, &blendInfo, &dynamicStateInfo, _pipeline_layout, _render_pass);

	auto res = device.createGraphicsPipeline(nullptr, pipelineInfo);
	switch(res.result) {
		case vk::Result::eSuccess:
			_pipeline = res.value;
			break;
		default:
			throw std::runtime_error("Erur");
	}
}

GraphicsPipeline::~GraphicsPipeline() {
	auto device = instance->device();
	if (_pipeline) {
		device.destroyPipeline(pipeline());
	}

	for(auto& l : _descriptor_set_layouts) {
		device.destroyDescriptorSetLayout(l);
	}
	_descriptor_set_layouts.clear();

	if (_render_pass) {
		device.destroyRenderPass(_render_pass);
	}

	if (_pipeline_layout) {
		device.destroyPipelineLayout(_pipeline_layout);
	}

	for(auto& m : modules) {
		device.destroyShaderModule(m);
	}
	modules.clear();
}
