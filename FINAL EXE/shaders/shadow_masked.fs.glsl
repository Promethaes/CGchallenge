#version 450

layout (binding = 0) uniform sampler2D a_Mask;
uniform vec2 a_OutputResolution;

out float gl_FragDepth;

void main() {
	if (texture(a_Mask, gl_FragCoord.xy / a_OutputResolution).r < 0.5f)
		gl_FragDepth = 0.0f;
	else
		gl_FragDepth = gl_FragCoord.z;
}