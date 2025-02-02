#version 460
#extension GL_EXT_nonuniform_qualifier: require

layout (location = 0) in VSOutput
{
	vec2 uvCoords;
} FSinput;


layout (location = 0) out vec4 outFragColor;


layout (binding = 1) uniform sampler2D textures[];

layout (push_constant) uniform PushConst
{
	uint diffuseId;
	uint specularId;
	uint emissiveId;
	uint normalId;
} pushConsts;


void main() 
{
	vec4 diffuse = vec4(1.0, 1.0, 1.0, 1.0);

	if(pushConsts.diffuseId > 0)
	{
		diffuse = texture(textures[pushConsts.diffuseId], FSinput.uvCoords);
	}
	vec4 specular = vec4(1.0, 0.1, 1.0, 1.0);

	if(pushConsts.specularId > 0)
	{	
		specular = texture(textures[pushConsts.specularId], FSinput.uvCoords);
	}
	vec4 emissive = vec4(1.0, 1.0, 1.0, 1.0);
	if(pushConsts.emissiveId > 0)
	{	
		emissive = texture(textures[pushConsts.emissiveId], FSinput.uvCoords);
	}
	vec4 normal = vec4(1.0, 1.0, 1.0, 1.0);
	if(pushConsts.normalId > 0)
	{	
		normal = texture(textures[pushConsts.normalId], FSinput.uvCoords);
	}


	outFragColor = diffuse + specular;
//	outFragColor = vec4(1.0);
}