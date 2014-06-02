#include "shared.h"
#include "common.hlsli"

struct VS_OUTPUT
{
	float4 pos		: POSITION;
	float3 normal	: NORMAL;
	float2 tex		: TEXCOORD0;
	float3 eyeDir	: TEXCOORD1;
};

cbuffer WORLD_CONST : register(b0)
{
	float4x4 viewproj : packoffset(c0);
	float4x4 world : packoffset(c4);
	float3 eyePos : packoffset(c8);
	uint skinCount : packoffset(c9);
};

StructuredBuffer<float4> g_pos : register (t1);
StructuredBuffer<float4> g_rot : register (t2);

cbuffer BONE_CONST : register(b1)
{
	float4x4 bones[35] : register(c0);
}

VS_OUTPUT main(float4 pos : POSITION,
	float3 normal : NORMAL,
	float2 tex : TEXCOORD0,
	float2 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	uint instance : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	static const uint columns = 64;

	float4 offset = float4(300 * (instance % columns - (columns - 0.5) / 2), 500 * (instance / columns), 0, 0);

	// skinning
	float w0 = weights.x;
	float w1 = weights.y;
	float w2 = 1.0 - w0 - w1;

	uint i0 = instance*skinCount + indices.x;
	uint i1 = instance*skinCount + indices.y;
	uint i2 = instance*skinCount + indices.z;

	Transform t0;
	Transform t1;
	Transform t2;
	t0.pos = g_pos[i0].xyz;
	t0.rot = g_rot[i0];
	t1.pos = g_pos[i1].xyz;
	t1.rot = g_rot[i1];
	t2.pos = g_pos[i2].xyz;
	t2.rot = g_rot[i2];

	float4x4 bone = TransformToMatrix(t0) * w0 + TransformToMatrix(t1) * w1 + TransformToMatrix(t2) * w2;

	pos = mul(pos, bone) + offset;
	normal = mul(normal, (float3x3)bone);

	// world-view
	float4 worldPos = mul(pos, world);

		output.pos = mul(worldPos, viewproj);
	output.normal = mul(normal, (float3x3)world);
	output.tex = tex;
	output.eyeDir = eyePos - worldPos.xyz;

	return output;
}