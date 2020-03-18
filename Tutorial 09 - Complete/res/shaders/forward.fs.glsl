#version 410

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec3 outNormal;

uniform sampler2D s_Albedo;

void main() {
	// Write the output
	outAlbedo = vec4(texture(s_Albedo, inUV).rgb * inColor.rgb, inColor.a);

	// Re-normalize our input, so that it is always length 1
	vec3 norm = normalize(inNormal);
	outNormal = (norm / 2) + vec3(0.5);
}