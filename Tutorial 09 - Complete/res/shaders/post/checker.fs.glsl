#version 440

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inScreenCoords ;

layout (location = 0) out vec4 outColor;

uniform sampler2D xImage;

uniform int xCheckerSize;
uniform vec3 xCheckerColor;

void main() {
	vec4 color = texture(xImage, inUV);
	float multiplier =
		mod(round(inScreenCoords.x / xCheckerSize) + round((inScreenCoords.y / xCheckerSize)), 2);
	color.rgb = (color.rgb * multiplier) + (xCheckerColor * (1 - multiplier));
	outColor = vec4(color.rgb, color.a);
}