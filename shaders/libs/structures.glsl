struct Vertex {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec4 uv;
	vec4 boneWeights;
	ivec4 boneIds;
	vec4 vertexColor;
};

struct Instance {
	mat4 model;
};

struct DownsampleParameters {
	uvec2 targetSize;
	uvec2 levelAndPadding;
};