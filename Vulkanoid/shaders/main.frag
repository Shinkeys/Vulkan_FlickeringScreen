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
	outFragColor = texture(textures[0], FSinput.uvCoords);
//	outFragColor = vec4(1.0);
}