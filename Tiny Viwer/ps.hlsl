struct VS_OUTPUT
{
	float4 pos		: POSITION;
	float3 normal	: NORMAL;
	float2 tex		: TEXCOORD0;
	float3 eyeDir	: TEXCOORD1;
};

texture diffuseTex;

sampler diffuseSampler =
sampler_state
{
	Texture = <diffuseTex>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

float4 main(VS_OUTPUT input) : COLOR0
{
	float3 lightDir = normalize(float3(-1, -1, 1));
	float3 normal = normalize(input.normal);
	float3 halfvec = normalize(lightDir + input.eyeDir);

	float ambient = 1.0;
	float diffuse = saturate(dot(normal, lightDir));
	float specular = pow(saturate(dot(halfvec, normal)), 20.0);

	float4 t = tex2D(diffuseSampler, input.tex);
	float4 color = t * (diffuse * 0.8 + ambient * 0.4) + specular * 0.5;

	color.a = 1;
	return color;
}