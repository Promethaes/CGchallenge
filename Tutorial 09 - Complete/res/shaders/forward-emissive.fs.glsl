#version 410

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outEmissive;

uniform sampler2D s_Albedo;
uniform sampler2D s_Emissive;

uniform float a_EmissiveStrength;

void main() {
	// Write the output
	outAlbedo = vec4(texture(s_Albedo, inUV).rgb * inColor.rgb, inColor.a);

	// Output the material's emissive light
	outEmissive = texture(s_Emissive, inUV).rgb * a_EmissiveStrength;

	// Re-normalize our input, so that it is always length 1
	vec3 norm = normalize(inNormal);
	outNormal = (norm / 2) + vec3(0.5);
}