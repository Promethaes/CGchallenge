#version 440

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inScreenCoords ;

layout (location = 0) out vec4 outColor;

uniform sampler2D xImage;
uniform ivec2 xScreenRes;
uniform mat3 a_Filter;

vec4 applyFilter(vec2 uv, vec2 res) {
	vec2 offset = vec2(1.33f) / res;
	vec4 result = vec4(0);
	for(int ix = -1; ix <= 1; ix++) {
		for (int iy = -1; iy <= 1; iy++) {
			result += texture(xImage, uv + (offset * vec2(ix, iy))) * a_Filter[iy + 1][ix + 1];
		}
	}
	return result;
}

void main() {
	outColor = applyFilter(inUV, xScreenRes);
}