//
//

#if !defined(_SHARED_H_)
#define _SHARED_H_

static const uint g_minInstance = 1 << 6;
static const uint g_maxInstance = 1 << 16;
static const uint g_maxBone = 64;
static const uint g_registerCount = 32;
static const uint g_threadCount = (4096 / g_registerCount) & 0xFFFFFFE0;
//static const uint g_threadCount = 192;
static const uint g_maxAnimationSet = 4;

struct TrackOffset
{
	uint posOffset;
	uint rotOffset;
};

struct InstanceParameter
{
	float startTime;
	uint animationSet;
};

struct BoneData
{
	uint parentBone;
	uint skinToBone;
	uint hierarchyWidth;
};

struct GlobalParam
{
	float t;
	uint g_instanceCount;
	uint g_instancePerGroup;
	uint g_boneCount;
	uint g_animationCount;
	uint g_hierarchyCount;
	uint g_hierarchyMaxWidth;
	uint g_skinCount;
};

struct AnimationSetInfo
{
	float period;
	uint referenceStart;
	uint animationStart;
	uint animationCount;
};

#endif //_SHARED_H_