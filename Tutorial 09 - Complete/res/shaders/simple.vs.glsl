#version 410
layout(location = 0) in vec3 inPosition;

uniform mat4 a_ModelViewProjection;

void main() {
	gl_Position = a_ModelViewProjection * vec4(inPosition, 1);
}