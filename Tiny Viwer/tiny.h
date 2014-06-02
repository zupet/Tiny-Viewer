//
//

#if !defined(_TINY_H_)
#define _TINY_H_

#include <vector>
#include <map>

typedef unsigned int uint;
typedef unsigned char byte;

struct float3
{
	float x, y, z;
};

struct float4
{
	float x, y, z, w;
};

struct KeyFloat3
{
	float Time;
	float3 Value;
};

struct KeyFloat4
{
	float Time;
	float4 Value;
};

#pragma pack(push)
#pragma pack(1)
struct TinyVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float weights[2];
	byte indices[4];
};

struct Skin
{
	char name[64];
	float3 translation;
	float4 rotation;
	float3 scale;
};

struct Bone
{
	char name[64];
	uint parent;
	float3 translation;
	float4 rotation;
	float3 scale;
};

struct Animation
{
	char name[64];
	uint translationStart;
	uint translationEnd;
	uint rotationStart;
	uint rotationEnd;
	uint scaleStart;
	uint scaleEnd;
};

struct AnimationSet
{
	char name[64];
	double period;
	uint animationStart;
	uint animationEnd;
};

#pragma pack(pop)

struct Tiny
{
	std::vector<Bone> m_bones;

	std::vector<TinyVertex> m_vertices;
	std::vector<WORD> m_indices;
	std::vector<Skin> m_skins;

	std::vector<AnimationSet> m_animationSets;
	std::vector<Animation> m_animations;

	std::map<std::string, size_t> m_nameToBone;
	std::vector<size_t> m_skinToBone;

	std::vector<KeyFloat3> m_translations;
	std::vector<KeyFloat4> m_rotations;
	std::vector<KeyFloat3> m_scales;

	void LoadMesh(LPCTSTR meshfile);
};

#endif //_TINY_H_