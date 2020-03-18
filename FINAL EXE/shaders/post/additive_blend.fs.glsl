#version 440

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inScreenCoords ;

layout (location = 0) out vec4 outColor;

uniform sampler2D xImage;

layout (binding = 1) uniform sampler2D a_Image1;
layout (binding = 2) uniform sampler2D a_Image2;

void main() {
	outColor = texture(a_Image1, inUV) + texture(a_Image2, inUV);	
}