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

layout(std430, set = 1, binding = 1) buffer INDIRECT {
	uint indirections[];
};


layout(location = 0) out vec3 vViewPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vTangent;
layout(location = 3) out vec3 vBiTangent;
layout(location = 4) out vec4 vUv;
layout(location = 5) out vec4 vVerexColor;
layout(location = 6) flat out ivec4 vMaterialMeshBatchId;

void main() {
	ObjectInstance instance = instances[indirections[gl_InstanceIndex]];
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
	vMaterialMeshBatchId = instance.materialMeshBatchId;
}