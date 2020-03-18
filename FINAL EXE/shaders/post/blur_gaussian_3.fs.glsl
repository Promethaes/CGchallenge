#version 440

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inScreenCoords ;

layout (location = 0) out vec4 outColor;

uniform sampler2D xImage;

uniform bool  isHorizontal;
uniform ivec2 xScreenRes;
//uniform vec2 xDirection;

const float weights[3] = float[](0.38774, 0.24477, 	0.06136);

void main() {
	vec2 off = 1.0 / textureSize(xImage, 0);
	if (isHorizontal) {
		off = off * vec2(1, 0);
	} else {
		off = off * vec2(0, 1);
	}
	vec4 result = texture(xImage, inUV) * weights[0];
	for(int i = 1; i < 3; i++) {
		result += texture(xImage, inUV + off * i) * weights[i];
		result += texture(xImage, inUV - off * i) * weights[i];
	}
	outColor = vec4(result.rgb, 1);
}