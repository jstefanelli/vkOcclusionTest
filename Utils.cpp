#include "Utils.h"
#include <fstream>
#include <stdexcept>

std::vector<char> readFile(const std::filesystem::path& path) {
	if (!exists(path)) {
		throw std::runtime_error("File not found");
	}

	std::ifstream file(path, std::ios::ate | std::ios::binary);

	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> data(size, 0U);

	file.read(data.data(), (std::streamsize) data.size());

	return data;
}

vk::ShaderModule createShaderModule(const std::filesystem::path& path, const vk::Device& device) {
	auto data = reinterpret_data<char, uint32_t>(readFile(path));

	return device.createShaderModule({ {}, data });
}