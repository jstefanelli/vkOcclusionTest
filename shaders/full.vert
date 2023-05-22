#version 450
#include "libs/structures.glsl"

precision highp float;
precision highp int;

layout(std430, set = 0, binding = 0) buffer VTX {
	Vertex vertices[];
};

layout(std430, set = 0, binding = 1) buffer INST {
	ModelInstance instances[];
};

layout(std140, set = 0, binding = 2) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
};

layout(location = 0) out vec3 vViewPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vTangent;
layout(location = 3) out vec3 vBiTangent;
layout(location = 4) out vec4 vUv;
layout(location = 5) out vec4 vVerexColor;
layout(location = 6) flat out ivec4 vMaterialId;

void main() {
	ModelInstance instance = instances[gl_InstanceIndex];
	Vertex vertex = vertices[gl_VertexIndex];

	vec4 viewPos = view * instance.model * vec4(vertex.position, 1.0);
	vViewPos = viewPos.xyz;
	gl_Position = projection * viewPos;
	mat4 inverseVM = inverse(view * instance.model);
	vec4 normal = inverseVM * vec4(vertex.normal, 0.0);
	vNormal = normal.xyz;
	vec4 tangent = inverseVM * vec4(vertex.tangent, 0.0);
	vTangent = tangent.xyz;
	vBiTangent = cross(vNormal, vTangent);
	vUv = vertex.uv;
	vVerexColor = vertex.vertexColor;
	vMaterialId = instance.materialId;
}