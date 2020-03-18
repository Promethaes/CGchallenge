#version 410

layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;

uniform sampler2D a_Sampler;

void main() {
	// Write the output
	outColor = texture(a_Sampler, inUV);
}