#version 450
#include "libs/structures.glsl"
/*
layout(std430, set = 2, binding = 0) buffer MATS {
	MaterialData materials[];
};

layout(set = 2, binding = 1) uniform sampler2D[4] textures;
*/

layout(location = 0) in vec3 vViewPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec3 vBiTangent;
layout(location = 4) in vec4 vUv;
layout(location = 5) in vec4 vVertexColor;
layout(location = 6) flat in ivec4 vMaterialMeshBatchId;

layout(location = 0) out vec4 oColor;
/*
vec4 calcRealColor() {
	MaterialData mat = materials[vMaterialMeshBatchId.x];

	vec4 textureFactors = vec4(mat.textureIds.x != 0 ? 1.0 : 0.0, mat.textureIds.y != 0 ? 1.0 : 0.0, mat.textureIds.z != 0 ? 1.0 : 0.0, mat.textureIds.w != 0 ? 1.0 : 0.0);
	ivec4 textureIds = clamp(mat.textureIds - ivec4(1, 1, 1, 1), ivec4(0, 0, 0, 0), ivec4(3, 3, 3, 3));

	vec4 val0 = texture(textures[textureIds.x], vUv.xy) * vec4(textureFactors.x);
	vec4 val1 = texture(textures[textureIds.y], vUv.xy) * vec4(textureFactors.y);
	vec4 val2 = texture(textures[textureIds.z], vUv.xy) * vec4(textureFactors.z);
	vec4 val3 = texture(textures[textureIds.w], vUv.xy) * vec4(textureFactors.w);

	return (val0 + val1 + val2 + val3) * mat.overrideColor * vVertexColor;
}
*/

void main() {
	//TODO: Actrually use materials and colors...
	oColor = vVertexColor;
}