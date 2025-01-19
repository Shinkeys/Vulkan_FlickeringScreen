#version 450
#extension GL_KHR_vulkan_glsl: enable


layout (location = 0) out vec4 outFragColor;


layout (set = 1, binding = 0) uniform sampler2D textures[];

void main() 
{
	outFragColor = vec4(vec3(1.0, 1.0, 1.0), 1.0);
}