#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in VSOutput
{
	vec2 uvCoords;
} FSinput;


layout (location = 0) out vec4 outFragColor;


layout (set = 0, binding = 1) uniform sampler2D textures[];

void main() 
{
	outFragColor = vec4(vec3(1.0), 1.0);
}