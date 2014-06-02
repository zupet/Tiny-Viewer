#include "shared.h"
#include "common.hlsli"

StructuredBuffer<InstanceParameter> g_instanceData : register(t0);
StructuredBuffer<TrackOffset> g_trackOffsets : register(t1);
StructuredBuffer<float> g_tTime : register(t2);
StructuredBuffer<float4> g_tValue : register(t3);
StructuredBuffer<float> g_rTime : register(t4);
StructuredBuffer<float4> g_rValue : register(t5);
StructuredBuffer<float4> g_invlocalPos : register(t6);
StructuredBuffer<float4> g_invlocalRot : register(t7);
StructuredBuffer<BoneData> g_boneData : register(t8);
StructuredBuffer<uint> g_animationToBone : register(t9);


RWStructuredBuffer<float4> g_skinPositions : register(u0);
RWStructuredBuffer<float4> g_skinRotations : register(u1);

cbuffer csGlobal : register(b0)
{
	float t;
	uint g_instanceCount;
	uint g_instancePerGroup;
	uint g_boneCount;
	uint g_animationCount;
	uint g_hierarchyCount;
	uint g_hierarchyMaxWidth;
	uint g_skinCount;
}

cbuffer csGlobal : register(b2)
{
	AnimationSetInfo g_animationSetInfos[g_maxAnimationSet];
}

groupshared BoneData boneData[g_maxBone];

groupshared float animationTime[g_threadCount];
groupshared uint referenceStart[g_threadCount];
groupshared uint animationStart[g_threadCount];
groupshared uint animationCount[g_threadCount];

groupshared float3 localPos[g_threadCount];
groupshared float4 localRot[g_threadCount];

[numthreads(g_threadCount, 1, 1)]
void main( uint3 groupID : SV_GroupID, uint thread : SV_GroupIndex )
{
	uint groupInstanceCount = min(g_instanceCount - groupID.x * g_instancePerGroup, g_instancePerGroup);
	uint boneCount = g_boneCount;

	if (thread < boneCount)
	{
		uint bone = thread;
		boneData[bone] = g_boneData[bone];
	}

	if (thread < groupInstanceCount)
	{
		uint instance = thread;
		uint globalInstance = groupID.x * g_instancePerGroup + instance;

		float startTime = g_instanceData[globalInstance].startTime;
		uint animationSet = g_instanceData[globalInstance].animationSet;

		animationTime[instance] = fmod(g_instanceData[globalInstance].startTime + t, g_animationSetInfos[animationSet].period) * 30;
		referenceStart[instance] = g_animationSetInfos[animationSet].referenceStart;
		animationStart[instance] = g_animationSetInfos[animationSet].animationStart;
		animationCount[instance] = g_animationSetInfos[animationSet].animationCount;
	}

	GroupMemoryBarrierWithGroupSync();

	if (thread < g_animationCount * groupInstanceCount)
	{
		uint animationInstance = thread / g_animationCount;
		uint animation = thread % g_animationCount;

		if (animation < animationCount[animationInstance])
		{
			uint iAnimation = animationStart[animationInstance] + animation;
			uint iBone = animationInstance * boneCount + g_animationToBone[iAnimation];

			float time = animationTime[animationInstance];

			uint frame = floor(time);

			uint iTrack = referenceStart[animationInstance] + frame * animationCount[animationInstance] + animation;

			uint pos = g_trackOffsets[iTrack].posOffset;

			float dPos = (time * 160.0 - g_tTime[pos]) / (g_tTime[pos + 1] - g_tTime[pos]);

			localPos[iBone] = lerp(g_tValue[pos], g_tValue[pos + 1], dPos);

			uint rot = g_trackOffsets[iTrack].rotOffset;

			float dRot = (time * 160.0 - g_rTime[rot]) / (g_rTime[rot + 1] - g_rTime[rot]);

			localRot[iBone] = lerp(g_rValue[rot], g_rValue[rot + 1], dRot);
		}
	}

	// local transform -> world transform
	uint columnInstance = thread / g_hierarchyMaxWidth;
	uint column = thread % g_hierarchyMaxWidth;
	uint tableBase = 1;

	for (uint hierarchy = 0; hierarchy < g_hierarchyCount; ++hierarchy)
	{
		GroupMemoryBarrierWithGroupSync();

		if (thread < g_hierarchyMaxWidth * groupInstanceCount)
		{
			uint width = boneData[hierarchy].hierarchyWidth;
			if (column < width)
			{
				uint bone = tableBase + column;
				uint parent = boneData[bone].parentBone;

				uint iParent = columnInstance * boneCount + parent;
				uint iBone = columnInstance * boneCount + bone;

				localPos[iBone] = Vec3Mul(localPos[iBone], localRot[iParent]) + localPos[iParent];
				localRot[iBone] = QuatMul(localRot[iParent], localRot[iBone]);
			}
			tableBase += width;
		}
	}

	GroupMemoryBarrierWithGroupSync();

	uint skinCount = g_skinCount;

	// map thread to skin
	if (thread < skinCount * groupInstanceCount)
	{
		uint skinInstance = thread / skinCount;
		uint skin = thread % skinCount;

		uint bone = boneData[skin].skinToBone;

		uint globalInstance = groupID.x * g_instancePerGroup + skinInstance;

		uint iBone = skinInstance * boneCount + bone;
		uint iSkin = globalInstance * skinCount + skin;

		g_skinPositions[iSkin].xyz = Vec3Mul(g_invlocalPos[skin].xyz, localRot[iBone]) + localPos[iBone];
		g_skinRotations[iSkin] = QuatMul(localRot[iBone], g_invlocalRot[skin]);
	}
}