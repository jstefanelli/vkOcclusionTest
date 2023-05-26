#ifndef align_16
#define align_16
#endif

#ifndef v3
#define v3 vec3
#endif

#ifndef v4
#define v4 vec4
#endif

#ifndef m4
#define m4 mat4
#endif

#ifndef i4
#define i4 ivec4
#endif

struct Vertex {
	align_16 v3 position;
	align_16 v3 normal;
	align_16 v3 tangent;
	align_16 v4 uv;
	v4 boneWeights;
	i4 boneIds;
	v4 vertexColor;
};

struct ObjectInstance {
	align_16 m4 model;
	i4 materialMeshBatchId; //last component is padding
	v4 bbCenter; //last component is padding
	v4 bbSize; //last component is padding
};

struct UniformData {
	align_16 m4 view;
	m4 projection;
};

struct MaterialData {
	align_16 v4 overrideColor;
	i4 textureIds;
};

struct DrawBatch {
	align_16 int meshId;
	int materialId;
	int amount;
	int padding;
};

struct DrawCommand {
	align_16 uint vertexCount;
	uint instanceCount;
	uint firstVertex;
	uint firstInstance;
};