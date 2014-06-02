// Tiny Viwer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Tiny Viwer.h"
#include "..\TRender\t_render.h"
#include "..\TRender\t_camera.h"
#include "..\TRender\t_render_dx11.h"

#include "tiny.h"

#pragma pack(push)
#pragma pack(1)
#include "shared.h"
#pragma pack(pop)

#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <numeric>

#include <float.h>

#include <directxpackedvector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

__int64 start, end, freq;
DWORD64 rdClock;
DWORD64 rdStart, rdEnd, rdLast;
std::deque<DWORD64> rdCompute;
std::deque<DWORD64> rdCopy;
std::deque<DWORD64> rdTotal;
uint g_indicesCount = 0;
uint g_timer = 0;

enum indices_type { indices_zero, indices_one, indices_small, indices_full };

static bool g_useGPU = true;
static bool g_playAnimation = true;
static bool g_doCopy = true;
static bool g_useWait = false;
static uint g_instanceCount = 1024;

static indices_type g_indicesType = indices_full;

struct WORLD_CONST
{
	XMMATRIX viewproj;
	XMMATRIX world;
	XMVECTOR eyePos;
	uint skinCount;
	uint dummy[3];
};

TRenderer* g_renderer = NULL;
TCamera g_camera;

XMVECTOR g_center = XMVectorSet(0, 0, 0, 0);
float g_radius = 4000;
float g_phi = XM_PI * 2 / 5;
float g_theta = -XM_PI / 2;// -XM_PI / 4;
XMMATRIX g_world;
DWORD g_tickStart;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	unsigned int control_word;
	_controlfp_s(&control_word, _DN_FLUSH, _MCW_DN);

	SetProcessAffinityMask(GetCurrentProcess(), 0x2);

	TCHAR szTitle[100];
	TCHAR szWindowClass[100];
	LoadString(hInstance, IDS_APP_TITLE, szTitle, 100);
	LoadString(hInstance, IDC_TINYVIWER, szWindowClass, 100);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYVIWER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_TINYVIWER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//
	Tiny tiny;
	tiny.LoadMesh(TEXT("tiny.tiny"));

	uint numBones = tiny.m_bones.size();
	uint numSkins = tiny.m_skins.size();
	uint numAnimationSets = tiny.m_animationSets.size();
	uint numTranslations = tiny.m_translations.size();
	uint numRotations = tiny.m_rotations.size();

	XMVECTOR localPos[g_maxBone];
	XMVECTOR localRot[g_maxBone];
	XMVECTOR invPos[g_maxBone];
	XMVECTOR invRot[g_maxBone];

	std::vector<InstanceParameter> instanceData(g_maxInstance);
	std::vector<BoneData> boneData(g_maxBone);
	GlobalParam globalParam;

	float t = 0;
	uint g_instancePerGroup = g_threadCount / numBones;
	uint g_boneCount = numBones;
	uint g_animationCount = 0;
	uint g_hierarchyCount = 0;
	uint g_hierarchyMaxWidth = 0;
	uint g_skinCount = numSkins;

	std::vector<uint> g_hierarchyWidth;
	std::vector<uint> g_boneToIndex(numBones);

	std::map<uint, uint> childIndex;
	childIndex[-1] = -1;
	std::vector<uint> parentMultiplyOrder;

	for (size_t i = 0; i < numBones; ++i)
	{
		const Bone& bone = tiny.m_bones[i];
		uint parent = bone.parent;
		uint child = childIndex[parent];
		childIndex[i] = child + 1;
		parentMultiplyOrder.push_back(child);
		g_hierarchyCount = max(g_hierarchyCount, child + 1);
	}

	uint index = 0;
	g_boneToIndex[index++] = 0;

	for (uint order = 0; order < g_hierarchyCount; ++order)
	{
		uint width = 0;
		for (uint b = 0; b < numBones; ++b)
		{
			if (parentMultiplyOrder[b] == order)
			{
				g_boneToIndex[b] = index++;
				++width;
			}
		}
		g_hierarchyWidth.push_back(width);
		g_hierarchyMaxWidth = max(g_hierarchyMaxWidth, width);
	}

	for (uint instance = 0; instance < g_maxInstance; ++instance)
	{
		uint animationSet = 1 + rand() % 3;

		instanceData[instance].startTime = rand() / 300.0f;
		instanceData[instance].animationSet = animationSet;
	}

	for (uint b = 0; b < numBones; ++b)
	{
		if (tiny.m_bones[b].parent != -1)
		{
			uint index = g_boneToIndex[b];
			boneData[index].parentBone = g_boneToIndex[tiny.m_bones[b].parent];
		}
		else
		{
			uint index = g_boneToIndex[b];
			boneData[index].parentBone = -1;
		}
	}

	uint totalWidth = 0;
	for (uint i = 0; i < g_hierarchyCount; ++i)
	{
		boneData[i].hierarchyWidth = g_hierarchyWidth[i];
		totalWidth += g_hierarchyWidth[i];
	}

	for (uint s = 0; s < numSkins; ++s)
	{
		const Skin& skin = tiny.m_skins[s];
		size_t skinToBone = tiny.m_skinToBone[s];
		boneData[s].skinToBone = g_boneToIndex[skinToBone];
	}

	XMVECTOR* skinPos = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* g_maxInstance * numSkins, 16);
	XMVECTOR* skinRot = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* g_maxInstance * numSkins, 16);
	XMVECTOR* tempPos = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* g_maxInstance * numSkins, 16);
	XMVECTOR* tempRot = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* g_maxInstance * numSkins, 16);

	float* tTime = (float*)_aligned_malloc(sizeof(float)* numTranslations, 16);
	float* rTime = (float*)_aligned_malloc(sizeof(float)* numRotations, 16);

	XMVECTOR* tValue = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* numTranslations, 16);
	XMVECTOR* rValue = (XMVECTOR*)_aligned_malloc(sizeof(XMVECTOR)* numRotations, 16);

	for (uint i = 0; i < numSkins; ++i)
	{
		invPos[i] = XMVectorSet(tiny.m_skins[i].translation.x, tiny.m_skins[i].translation.y, tiny.m_skins[i].translation.z, 0);
		invRot[i] = XMVectorSet(tiny.m_skins[i].rotation.x, tiny.m_skins[i].rotation.y, tiny.m_skins[i].rotation.z, tiny.m_skins[i].rotation.w);
	}

	for (uint i = 0; i < numTranslations; ++i)
	{
		tTime[i] = tiny.m_translations[i].Time;
		tValue[i] = XMVectorSet(tiny.m_translations[i].Value.x, tiny.m_translations[i].Value.y, tiny.m_translations[i].Value.z, 0);
	}

	for (uint i = 0; i < numRotations; ++i)
	{
		rTime[i] = tiny.m_rotations[i].Time;
		rValue[i] = XMVectorSet(tiny.m_rotations[i].Value.x, tiny.m_rotations[i].Value.y, tiny.m_rotations[i].Value.z, tiny.m_rotations[i].Value.w);
	}


	std::vector<TrackOffset> trackOffsets;
	std::vector<AnimationSetInfo> animationSetInfos(g_maxAnimationSet);
	std::vector<uint> animationToBone(g_maxAnimationSet * g_maxBone);

	uint referenceStart = 0;

	for (uint i = 0; i < numAnimationSets; ++i)
	{
		const AnimationSet& animationSet = tiny.m_animationSets[i];
		uint numAnimation = animationSet.animationEnd - animationSet.animationStart;

		std::vector<uint> posIter;
		std::vector<uint> rotIter;
		for (uint j = 0; j < numAnimation; ++j)
		{
			const Animation& animation = tiny.m_animations[animationSet.animationStart + j];

			posIter.push_back(animation.translationStart);
			rotIter.push_back(animation.rotationStart);

			animationToBone[animationSet.animationStart + j] = g_boneToIndex[tiny.m_nameToBone[animation.name]];
		}

		uint numFrame = (uint)floor(animationSet.period * 30.0 + 0.5);

		animationSetInfos[i].period = (float)animationSet.period;
		animationSetInfos[i].referenceStart = referenceStart;
		animationSetInfos[i].animationStart = animationSet.animationStart;
		animationSetInfos[i].animationCount = numAnimation;

		for (uint frame = 0; frame < numFrame; ++frame)
		{
			for (uint j = 0; j < numAnimation; ++j)
			{
				while (tiny.m_translations[posIter[j] + 1].Time <= frame * 160) ++posIter[j];
				while (tiny.m_rotations[rotIter[j] + 1].Time <= frame * 160) ++rotIter[j];

				TrackOffset trackOffset;
				trackOffset.posOffset = posIter[j];
				trackOffset.rotOffset = rotIter[j];
				trackOffsets.push_back(trackOffset);
			}
		}
		referenceStart += numAnimation * numFrame;

		g_animationCount = max(g_animationCount, numAnimation);
	}

#pragma pack(push)
#pragma pack(1)
	struct Vertex
	{
		float x, y, z;
		XMHALF4 normal;
		XMHALF2 tex;
		XMHALF2 weights;
		BYTE indices[4];
	};
#pragma pack(pop)

	std::vector<Vertex> vertices;

	for (uint i = 0; i < tiny.m_vertices.size(); ++i)
	{
		Vertex vertex;
		vertex.x = tiny.m_vertices[i].x;
		vertex.y = tiny.m_vertices[i].y;
		vertex.z = tiny.m_vertices[i].z;
		vertex.normal = XMHALF4(tiny.m_vertices[i].nx, tiny.m_vertices[i].ny, tiny.m_vertices[i].nz, 0);
		vertex.tex = XMHALF2(tiny.m_vertices[i].u, tiny.m_vertices[i].v);
		vertex.weights = XMHALF2(tiny.m_vertices[i].weights[0], tiny.m_vertices[i].weights[1]);
		vertex.indices[0] = tiny.m_vertices[i].indices[0];
		vertex.indices[1] = tiny.m_vertices[i].indices[1];
		vertex.indices[2] = tiny.m_vertices[i].indices[2];
		vertex.indices[3] = tiny.m_vertices[i].indices[3];
		vertices.push_back(vertex);
	}

	std::vector<WORD> indicesSmall;

	for (uint i = 0; i < tiny.m_indices.size(); i += 3 * 10)
	{
		indicesSmall.push_back(tiny.m_indices[i + 0]);
		indicesSmall.push_back(tiny.m_indices[i + 1]);
		indicesSmall.push_back(tiny.m_indices[i + 2]);
	}

	// 
	g_renderer = new TRendererDX11();
	g_renderer->Init(hWnd, TMSAA_MAXIMUM);

	g_center = XMVectorSet(0, 0, 250.f, 0);

	RECT windowRect;
	GetClientRect(hWnd, &windowRect);

	XMVECTOR eye = g_center + XMVectorSet(cosf(g_theta) * sinf(g_phi), sinf(g_theta) * sinf(g_phi), cosf(g_phi), 0) * g_radius;
	g_camera.SetView(eye, g_center, XMVectorSet(0, 0, 1, 0));

	float aspect = (float)(windowRect.right - windowRect.left) / (windowRect.bottom - windowRect.top);
	g_camera.SetProj(XM_PI / 4, aspect, 1.0f, g_radius * 1000);

	g_world = XMMatrixIdentity();

	TDeclDesc declMesh[] =
	{
		{ TYPE_POSITION, FORMAT_FLOAT3, 0, 0, 0, false },
		{ TYPE_NORMAL, FORMAT_HALF4, 0, 0, 12, false },
		{ TYPE_TEXTURE, FORMAT_HALF2, 0, 0, 20, false },
		{ TYPE_WEIGHT, FORMAT_HALF2, 0, 0, 24, false },
		{ TYPE_BONE, FORMAT_BYTE4, 0, 0, 28, false },
	};

	TDeclDesc declBone[1] =
	{
		{ TYPE_POSITION, FORMAT_FLOAT3, 0, 0, 0 },
	};

	TShader* g_vsMesh = g_renderer->CreateVertexShader(TEXT("vs.cso"));
	TShader* g_psMesh = g_renderer->CreatePixelShader(TEXT("ps.cso"));
	TShader* g_csMesh = g_renderer->CreateComputeShader(TEXT("cs.cso"));

	TDecl* g_declMesh = g_renderer->CraeteVertexDecl(g_vsMesh, declMesh, _countof(declMesh));

	TBuffer* g_vsConst = g_renderer->CreateConstantBuffer(sizeof(WORLD_CONST));

	TBuffer* g_vbMesh = g_renderer->CreateVertexBuffer(sizeof(Vertex), vertices.size());
	TBuffer* g_ibFull = g_renderer->CreateIndexBuffer(sizeof(WORD)* tiny.m_indices.size());
	TBuffer* g_ibSmall = g_renderer->CreateIndexBuffer(sizeof(WORD)* indicesSmall.size());

	TBuffer* g_globalParam = g_renderer->CreateConstantBuffer(sizeof(GlobalParam));
	TBuffer* g_animationSetInfos = g_renderer->CreateConstantBuffer(sizeof(AnimationSetInfo)* g_maxAnimationSet);

	TBuffer* g_instanceData = g_renderer->CreateStructuredBuffer(sizeof(InstanceParameter), g_maxInstance);
	TBuffer* g_trackOffsets = g_renderer->CreateStructuredBuffer(sizeof(TrackOffset), trackOffsets.size());
	TBuffer* g_tTime = g_renderer->CreateStructuredBuffer(sizeof(float), numTranslations);
	TBuffer* g_tValue = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), numTranslations);
	TBuffer* g_rTime = g_renderer->CreateStructuredBuffer(sizeof(float), numRotations);
	TBuffer* g_rValue = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), numRotations);
	TBuffer* g_invPositions = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), numSkins);
	TBuffer* g_invRotations = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), numSkins);
	TBuffer* g_boneData = g_renderer->CreateStructuredBuffer(sizeof(BoneData), g_maxBone);
	TBuffer* g_animationToBone = g_renderer->CreateStructuredBuffer(sizeof(uint), g_maxAnimationSet * g_maxBone);

	TBuffer* g_skinPositions = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), g_maxInstance * numSkins);
	TBuffer* g_skinRotations = g_renderer->CreateStructuredBuffer(sizeof(XMVECTOR), g_maxInstance * numSkins);

	TTexture* g_texture = g_renderer->CreateTexture(TEXT("Tiny_skin.bmp"));

	TQuery* g_query = g_renderer->CreateQuery();

	// initialize

	globalParam.t = 0;
	globalParam.g_instanceCount = g_instanceCount;
	globalParam.g_instancePerGroup = g_instancePerGroup;
	globalParam.g_boneCount = g_boneCount;
	globalParam.g_animationCount = g_animationCount;
	globalParam.g_hierarchyCount = g_hierarchyCount;
	globalParam.g_hierarchyMaxWidth = g_hierarchyMaxWidth;
	globalParam.g_skinCount = numSkins;

	g_renderer->UpdateVertexBuffer(g_vbMesh, &vertices[0], sizeof(Vertex)* vertices.size());
	g_renderer->UpdateIndexBuffer(g_ibFull, &tiny.m_indices[0], sizeof(WORD)* tiny.m_indices.size());
	g_renderer->UpdateIndexBuffer(g_ibSmall, &indicesSmall[0], sizeof(WORD)* indicesSmall.size());

	g_renderer->UpdateConstantBuffer(g_globalParam, &globalParam, sizeof(GlobalParam));
	g_renderer->UpdateConstantBuffer(g_animationSetInfos, &animationSetInfos[0], sizeof(AnimationSetInfo)* animationSetInfos.size());
	
	
	g_renderer->UpdateStructuredBuffer(g_instanceData, &instanceData[0], sizeof(InstanceParameter)* instanceData.size());
	g_renderer->UpdateStructuredBuffer(g_trackOffsets, &trackOffsets[0], sizeof(TrackOffset)* trackOffsets.size());
	g_renderer->UpdateStructuredBuffer(g_tTime, &tTime[0], sizeof(float)* numTranslations);
	g_renderer->UpdateStructuredBuffer(g_tValue, &tValue[0], sizeof(XMVECTOR)* numTranslations);
	g_renderer->UpdateStructuredBuffer(g_rTime, &rTime[0], sizeof(float)* numRotations);
	g_renderer->UpdateStructuredBuffer(g_rValue, &rValue[0], sizeof(XMVECTOR)* numRotations);
	g_renderer->UpdateStructuredBuffer(g_invPositions, invPos, sizeof(XMVECTOR)* numSkins);
	g_renderer->UpdateStructuredBuffer(g_invRotations, invRot, sizeof(XMVECTOR)* numSkins);
	g_renderer->UpdateStructuredBuffer(g_boneData, &boneData[0], sizeof(BoneData)* boneData.size());
	g_renderer->UpdateStructuredBuffer(g_animationToBone, &animationToBone[0], sizeof(uint)* animationToBone.size());

	g_renderer->SetComputeShader(g_csMesh);
	g_renderer->SetComputeConstant(0, g_globalParam);
	g_renderer->SetComputeConstant(2, g_animationSetInfos);
	g_renderer->SetComputeStructureReadOnly(0, g_instanceData);
	g_renderer->SetComputeStructureReadOnly(1, g_trackOffsets);
	g_renderer->SetComputeStructureReadOnly(2, g_tTime);
	g_renderer->SetComputeStructureReadOnly(3, g_tValue);
	g_renderer->SetComputeStructureReadOnly(4, g_rTime);
	g_renderer->SetComputeStructureReadOnly(5, g_rValue);
	g_renderer->SetComputeStructureReadOnly(6, g_invPositions);
	g_renderer->SetComputeStructureReadOnly(7, g_invRotations);
	g_renderer->SetComputeStructureReadOnly(8, g_boneData);
	g_renderer->SetComputeStructureReadOnly(9, g_animationToBone);

	g_renderer->SetVertexConstant(0, g_vsConst);
	g_renderer->SetVertexDecl(g_declMesh);
	g_renderer->SetVertexBuffer(0, g_vbMesh);
	g_renderer->SetVertexShader(g_vsMesh);
	g_renderer->SetPixelShader(g_psMesh);
	g_renderer->SetTexture(0, g_texture);

	g_tickStart = GetTickCount();

	//
	rdLast = __rdtsc();

	MSG msg = { 0, };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// update animation
			float tick = (GetTickCount() - g_tickStart) / 1000.0f;

			if (g_playAnimation)
			{
				if (g_useGPU)
				{
					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdStart = __rdtsc();

					globalParam.t = tick;
					globalParam.g_instanceCount = g_instanceCount;

					g_renderer->UpdateConstantBuffer(g_globalParam, &globalParam, sizeof(GlobalParam));

					g_renderer->SetComputeStructureReadWrite(0, g_skinPositions);
					g_renderer->SetComputeStructureReadWrite(1, g_skinRotations);

					g_renderer->Dispatch((g_instanceCount + g_instancePerGroup - 1) / g_instancePerGroup, 1, 1);

					g_renderer->SetComputeStructureReadWrite(0, NULL);
					g_renderer->SetComputeStructureReadWrite(1, NULL);

					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdCompute.push_back(__rdtsc() - rdStart);

					rdCopy.push_back(0);

					//std::vector<XMVECTOR> tempPos(g_maxInstance * numSkins);
					//std::vector<XMVECTOR> tempRot(g_maxInstance * numSkins);
					//g_renderer->CopyResource(g_skinPositions, &tempPos[0]);
					//g_renderer->CopyResource(g_skinRotations, &tempRot[0]);
				}
				else
				{
					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdStart = __rdtsc();

					for (uint instance = 0; instance < g_instanceCount; ++instance)
					{
						uint animationSet = instanceData[instance].animationSet;

						float time = fmod(instanceData[instance].startTime + tick, animationSetInfos[animationSet].period) * 30.0f;

						uint frame = (uint)floor(time);

						uint animationCount = animationSetInfos[animationSet].animationCount;
						uint animationBase = animationSetInfos[animationSet].referenceStart + frame * animationCount;
						uint* animationToBoneBase = &animationToBone[animationSetInfos[animationSet].animationStart];

						for (uint animation = 0; animation < animationCount; ++animation)
						{
							uint iAnimation = animationBase + animation;

							uint pos = trackOffsets[iAnimation].posOffset;
							uint rot = trackOffsets[iAnimation].rotOffset;

							float dPos = (time * 160.0f - tTime[pos]) / (tTime[pos + 1] - tTime[pos]);
							float dRot = (time * 160.0f - rTime[rot]) / (rTime[rot + 1] - rTime[rot]);

							uint iBone = animationToBoneBase[animation];

							localPos[iBone] = XMVectorLerp(tValue[pos], tValue[pos + 1], dPos);
							localRot[iBone] = XMVectorLerp(rValue[rot], rValue[rot + 1], dRot);
						}

						for (uint i = 0; i < numBones; ++i)
						{
							uint iBone = i;

							uint parent = boneData[i].parentBone;
							if (parent != -1)
							{
								uint iParent = parent;

								localPos[iBone] = XMVector3Rotate(localPos[iBone], localRot[iParent]) + localPos[iParent];
								localRot[iBone] = XMQuaternionMultiply(localRot[iBone], localRot[iParent]);
							}
						}

						// matrix
						for (size_t i = 0; i < numSkins; ++i)
						{
							uint iSkin = instance * numSkins + i;
							uint iBone = boneData[i].skinToBone;

							skinPos[iSkin] = XMVector3Rotate(invPos[i], localRot[iBone]) + localPos[iBone];
							skinRot[iSkin] = XMQuaternionMultiply(invRot[i], localRot[iBone]);
						}
					}

					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdCompute.push_back(__rdtsc() - rdStart);
				}
			}

			if (g_doCopy)
			{
				if (g_useGPU)
				{
					rdCopy.push_back(0);
				}
				else
				{
					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdStart = __rdtsc();

					g_renderer->UpdateStructuredBuffer(g_skinPositions, &skinPos[0], sizeof(XMVECTOR)* g_instanceCount * numSkins);
					g_renderer->UpdateStructuredBuffer(g_skinRotations, &skinRot[0], sizeof(XMVECTOR)* g_instanceCount * numSkins);

					if (g_useWait) g_renderer->WaitForQuery(g_query);
					rdCopy.push_back(__rdtsc() - rdStart);
				}

			}

			WORLD_CONST vsConst;
			vsConst.viewproj = XMMatrixTranspose(g_camera.GetView() * g_camera.GetProj());
			vsConst.world = XMMatrixTranspose(g_world);
			vsConst.eyePos = g_camera.GetEye();
			vsConst.skinCount = numSkins;

			g_renderer->UpdateConstantBuffer(g_vsConst, &vsConst, sizeof(WORLD_CONST));

			if (g_renderer->BeginScene())
			{
				g_renderer->Clear();

				g_renderer->SetVertexStructureReadOnly(1, g_skinPositions);
				g_renderer->SetVertexStructureReadOnly(2, g_skinRotations);

				TBuffer* indices = NULL;

				switch (g_indicesType)
				{
				case indices_one:
					indices = g_ibFull;
					g_indicesCount = 1;
					break;
				case indices_small:
					indices = g_ibSmall;
					g_indicesCount = indicesSmall.size();
					break;
				case indices_full:
					indices = g_ibFull;
					g_indicesCount = tiny.m_indices.size();
					break;
				}

				g_renderer->SetIndexBuffer(indices);
				g_renderer->DrawIndexedInstanced(g_indicesCount / 3, g_instanceCount);

				g_renderer->SetVertexStructureReadOnly(1, NULL);
				g_renderer->SetVertexStructureReadOnly(2, NULL);

				g_renderer->EndScene();
				g_renderer->Present();
			}

			DWORD64 rdTime = __rdtsc();
			rdTotal.push_back(rdTime - rdLast);
			rdLast = rdTime;

			if (rdCompute.size() > 30) rdCompute.pop_front();
			if (rdCopy.size() > 30) rdCopy.pop_front();
			if (rdTotal.size() > 30) rdTotal.pop_front();

			TCHAR temp[256];
			_stprintf_s(temp, _T("Tiny Viewer - %d faces, gpu %s, animation %s, copy %s, wait %s, %d instaces, %fms total , %fms compute, %fms copy"), 
				g_indicesCount / 3,
				g_useGPU ? TEXT("yes") : TEXT("no"),
				g_playAnimation ? TEXT("yes") : TEXT("no"),
				g_doCopy ? TEXT("yes") : TEXT("no"),
				g_useWait ? TEXT("yes") : TEXT("no"),
				g_instanceCount,
				std::accumulate(rdTotal.begin(), rdTotal.end(), (DWORD64)0) / double(rdClock) / rdTotal.size(),
				std::accumulate(rdCompute.begin(), rdCompute.end(), (DWORD64)0) / double(rdClock) / rdCompute.size(),
				std::accumulate(rdCopy.begin(), rdCopy.end(), (DWORD64)0) / double(rdClock) / rdCopy.size());

			SetWindowText(hWnd, temp);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int dx, dy;
	static int xLast, yLast;
	static int captures = 0;

	switch (message)
	{
	case WM_CREATE:
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		rdStart = __rdtsc();
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
		Sleep(100);
		rdEnd = __rdtsc();
		QueryPerformanceCounter((LARGE_INTEGER*)&end);
		rdClock = (rdEnd - rdStart) * freq / ((end - start) * 1000);

		//SetTimer(hWnd, 0, 1000 * 5, NULL);
		break;

	case WM_TIMER:
		{
			FILE* file;
			if (fopen_s(&file, "log.txt", "at") == 0)
			{
				_ftprintf_s(file, _T("%d faces	gpu %s	animation %s	copy %s	wait %s	%d instaces	%f total	%f compute	%f copy\n"),
					g_indicesCount / 3,
					g_useGPU ? TEXT("yes") : TEXT("no"),
					g_playAnimation ? TEXT("yes") : TEXT("no"),
					g_doCopy ? TEXT("yes") : TEXT("no"),
					g_useWait ? TEXT("yes") : TEXT("no"),
					g_instanceCount,
					std::accumulate(rdTotal.begin(), rdTotal.end(), (DWORD64)0) / double(rdClock) / rdTotal.size(),
					std::accumulate(rdCompute.begin(), rdCompute.end(), (DWORD64)0) / double(rdClock) / rdCompute.size(),
					std::accumulate(rdCopy.begin(), rdCopy.end(), (DWORD64)0) / double(rdClock) / rdCopy.size());

				fclose(file);
			}


			g_useGPU = g_timer & 0x01 ? true : false;
			g_playAnimation = g_timer & 0x02 ? true : false;
			g_doCopy = g_timer & 0x04 ? true : false;
			g_instanceCount = 1024 << ((g_timer >> 3) & 0x07);
			++g_timer;
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			switch (LOWORD(wParam))
			{
			case ID_FILE_PLAYANIMATION:
				g_playAnimation = true;
				break;
			case ID_FILE_STOPANIMATION:
				g_playAnimation = false;
				break;
			case ID_FILE_USEGPU:
				g_useGPU = true;
				break;
			case ID_FILE_USECPU:
				g_useGPU = false;
				break;
			case ID_FILE_COPYBUFFER:
				g_doCopy = true;
				break;
			case ID_FILE_DONTCOPYBUFFER:
				g_doCopy = false;
				break;
			case ID_FILE_USEWAIT:
				g_useWait = true;
				break;
			case ID_FILE_DONT_WAIT:
				g_useWait = false;
				break;
			case ID_FILE_FULLINDICES:
				g_indicesType = indices_full;
				break;
			case ID_FILE_1_OVER_10_INDICES:
				g_indicesType = indices_small;
				break;
			case ID_FILE_1INDICES:
				g_indicesType = indices_one;
				break;
			}
			rdCompute.clear();
			rdCopy.clear();
			rdTotal.clear();
			rdLast = __rdtsc();
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			g_instanceCount = max(g_instanceCount >> 1, g_minInstance);
			break;
		case VK_RIGHT:
			g_instanceCount = min(g_instanceCount << 1, g_maxInstance);
			break;
		}
		break;

	case WM_MOUSEWHEEL:
		g_radius -= ((short)HIWORD(wParam) / WHEEL_DELTA) * 100.0f;
		g_camera.SetEye(g_center + XMVectorSet(cosf(g_theta) * sinf(g_phi), sinf(g_theta) * sinf(g_phi), cosf(g_phi), 0) * g_radius);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (captures++ == 0) SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (--captures == 0) ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		dx = LOWORD(lParam) - xLast;
		dy = HIWORD(lParam) - yLast;
		if (wParam & MK_LBUTTON)
		{
			XMMATRIX x, y;
			x = XMMatrixRotationAxis(g_camera.GetViewY(), -dx*0.01f);
			y = XMMatrixRotationAxis(g_camera.GetViewX(), -dy*0.01f);
			g_world = g_world * x * y;
		}
		if (wParam & MK_RBUTTON)
		{
			g_theta -= dx*0.01f;
			g_phi = min(XM_PI*0.99f, max(XM_PI*0.01f, g_phi - dy*0.01f));
			g_camera.SetEye(g_center + XMVectorSet(cosf(g_theta) * sinf(g_phi), sinf(g_theta) * sinf(g_phi), cosf(g_phi), 0) * g_radius);

		}
		xLast = LOWORD(lParam);
		yLast = HIWORD(lParam);
		break;

	case WM_SIZE:
		g_camera.SetAspect((float)LOWORD(lParam) / HIWORD(lParam));
		if (g_renderer)
			g_renderer->Reset(hWnd);
		rdCompute.clear();
		rdCopy.clear();
		rdTotal.clear();
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
