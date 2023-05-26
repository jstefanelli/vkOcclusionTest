#version 450
#include "libs/structures.glsl"

precision highp float;
precision highp int;

layout(std430, set = 0, binding = 0) buffer VTX {
	Vertex vertices[];
};

layout(std140, set = 0, binding = 1) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
};

layout(std430, set = 1, binding = 0) buffer INST {
	ObjectInstance instances[];
};


layout(location = 0) out vec4 vVertexColor;

void main() {
	Vertex v = vertices[gl_VertexIndex];
	mat4 model = instances[gl_InstanceIndex].model;

	gl_Position = projection * view * model * vec4(v.position, 1.0);
	vVertexColor = v.vertexColor;
}