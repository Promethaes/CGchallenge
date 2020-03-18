#version 410


layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;

uniform sampler2D s_Albedo;

uniform vec3  a_CameraPos;

uniform vec3  a_AmbientColor;
uniform float a_AmbientPower;
uniform float a_MatShininess;

const int MAX_LIGHTS = 16;
struct Light{
	vec3  Pos;
	vec3  Color;
	float Attenuation;
};
uniform Light a_Lights[MAX_LIGHTS];
uniform int a_EnabledLights;

vec3 ResolvePointLight(Light light, vec3 norm) {
	// Determine the direction from the position to the light
	vec3 toLight = light.Pos - inWorldPos;

	// Determine the distance to the light (used for attenuation later)
	float distToLight = length(toLight);
	// Normalize our toLight vector
	toLight = normalize(toLight);

	// Determine the direction between the camera and the pixel
	vec3 viewDir = normalize(a_CameraPos - inWorldPos);

	// Calculate the halfway vector between the direction to the light and the direction to the eye
	vec3 halfDir = normalize(toLight + viewDir);

	// Our specular power is the angle between the the normal and the half vector, raised
	// to the power of the light's shininess
	float specPower = pow(max(dot(norm, halfDir), 0.0), a_MatShininess);

	// Finally, we can calculate the actual specular factor
	vec3 specOut = specPower * light.Color;

	// Calculate our diffuse factor, this is essentially the angle between
	// the surface and the light
	float diffuseFactor = max(dot(norm, toLight), 0);
	// Calculate our diffuse output
	vec3  diffuseOut = diffuseFactor * light.Color;

	// We will use a modified form of distance squared attenuation, which will avoid divide
	// by zero errors and allow us to control the light's attenuation via a uniform
	float attenuation = 1.0 / (1.0 + light.Attenuation * pow(distToLight, 2));

	return attenuation * (diffuseOut + specOut);
}

void main() {
	// Re-normalize our input, so that it is always length 1
	vec3 norm = normalize(inNormal);
	
	// Our ambient is simply the color times the ambient power
	vec3 result = a_AmbientColor * a_AmbientPower;

	// Iterate over all the lights and sum their influence on the final result
	for (int i = 0; (i < a_EnabledLights) && (i < MAX_LIGHTS); i++) {
		result += ResolvePointLight(a_Lights[i], norm);
	}

	// Multiply the lighting by the object's color
	result = result * texture(s_Albedo, inUV).rgb * inColor.rgb;

	// TODO: gamma correction

	// Write the output
	outColor = vec4(result, inColor.a);
	outNormal = (norm / 2) + vec3(0.5);
}