#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (set = 1, binding = 1) uniform UBO
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}