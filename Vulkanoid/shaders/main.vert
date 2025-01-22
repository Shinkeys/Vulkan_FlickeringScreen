#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

struct VSOutput
{
	vec2 uvCoords;
};

layout (location = 0) out VSOutput VSOut;

void main() 
{
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
	VSOut.uvCoords = inTexCoord;
}