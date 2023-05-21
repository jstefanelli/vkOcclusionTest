#version 450

layout(location = 0) out vec4 oColor;

layout(location = 0) in vec4 vVertexColor;

void main() {
	oColor = vVertexColor;
}