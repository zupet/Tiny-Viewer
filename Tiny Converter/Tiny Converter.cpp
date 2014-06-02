// Tiny Converter.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Tiny Converter.h"

#include <list>
#include <vector>
#include <stack>
#include <deque>

#include <atlbase.h>

#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

typedef unsigned int uint;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

#pragma pack(push)
#pragma pack(1)
struct TinyVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float weights[2];
	DWORD indices;
};

struct Skin
{
	char name[64];
	D3DXVECTOR3 translation;
	D3DXQUATERNION rotation;
	D3DXVECTOR3 scale;
};

struct Bone
{
	char name[64];
	uint parent;
	D3DXVECTOR3 translation;
	D3DXQUATERNION rotation;
	D3DXVECTOR3 scale;
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

std::vector<Bone> m_bones;

std::vector<TinyVertex> m_vertices;
std::vector<WORD> m_indices;
std::vector<Skin> m_skins;

std::vector<D3DXKEY_VECTOR3> m_translations;
std::vector<D3DXKEY_QUATERNION> m_rotations;
std::vector<D3DXKEY_VECTOR3> m_scales;

std::vector<Animation> m_animations;
std::vector<AnimationSet> m_animationSets;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AllocateHierarchy : public ID3DXAllocateHierarchy
{
public:
	STDMETHOD(CreateFrame)(LPCSTR Name, LPD3DXFRAME *ppNewFrame)
	{
		m_frameList.push_back(new D3DXFRAME());

		*ppNewFrame = m_frameList.back();

		memset(*ppNewFrame, 0, sizeof(D3DXFRAME));

		if (Name == NULL)
			Name = "No Name";

		size_t len = strlen(Name) + 1;
		(*ppNewFrame)->Name = new char[len];
		strcpy_s((*ppNewFrame)->Name, len, Name);

		return S_OK;
	}

	STDMETHOD(DestroyFrame)(LPD3DXFRAME pFrameToFree)
	{
		m_frameList.remove(pFrameToFree);

		return S_OK;
	}

	STDMETHOD(CreateMeshContainer)(LPCSTR Name, CONST D3DXMESHDATA *pMeshData, CONST D3DXMATERIAL *pMaterials, CONST D3DXEFFECTINSTANCE *pEffectInstances, DWORD NumMaterials, CONST DWORD *pAdjacency, LPD3DXSKININFO pSkinInfo, LPD3DXMESHCONTAINER *ppNewMeshContainer)
	{
		m_meshContainerList.push_back(new D3DXMESHCONTAINER());
		*ppNewMeshContainer = m_meshContainerList.back();

		memset(*ppNewMeshContainer, 0, sizeof(LPD3DXMESHCONTAINER));

		if (Name == NULL)
			Name = "No Name";

		size_t len = strlen(Name) + 1;
		(*ppNewMeshContainer)->Name = new char[len];
		strcpy_s((*ppNewMeshContainer)->Name, len, Name);

		(*ppNewMeshContainer)->MeshData = *pMeshData;
		if (pMeshData->Type == D3DXMESHTYPE_MESH && pMeshData->pMesh && pSkinInfo)
		{

			DWORD dwMaxNumFaceInfls;
			DWORD dwNumAttrGroups;
			CComPtr<ID3DXBuffer> pBufBoneCombos;
			pSkinInfo->ConvertToIndexedBlendedMesh(pMeshData->pMesh, 0, 255, pAdjacency, NULL, NULL, NULL, &dwMaxNumFaceInfls, &dwNumAttrGroups, &pBufBoneCombos, &(*ppNewMeshContainer)->MeshData.pMesh);

			DWORD numSkins = pSkinInfo->GetNumBones();
			for (DWORD i = 0; i < numSkins; ++i)
			{
				Skin skin;
				strcpy_s(skin.name, pSkinInfo->GetBoneName(i));
				D3DXMATRIX offset = *pSkinInfo->GetBoneOffsetMatrix(i);
				D3DXMatrixDecompose(&skin.scale, &skin.rotation, &skin.translation, &offset);
				m_skins.push_back(skin);
			}

			
		}


		//(*ppNewMeshContainer)->pMaterials;
		//(*ppNewMeshContainer)->pEffects;
		//(*ppNewMeshContainer)->NumMaterials;
		//(*ppNewMeshContainer)->pAdjacency;
		(*ppNewMeshContainer)->pSkinInfo = pSkinInfo;
		if ((*ppNewMeshContainer)->pSkinInfo)
			(*ppNewMeshContainer)->pSkinInfo->AddRef();

		return S_OK;
	}

	STDMETHOD(DestroyMeshContainer)(LPD3DXMESHCONTAINER pMeshContainerToFree)
	{
		m_meshContainerList.remove(pMeshContainerToFree);

		return S_OK;
	}

protected:
	std::list<D3DXFRAME*> m_frameList;
	std::list<D3DXMESHCONTAINER*> m_meshContainerList;
};

void ExtractSkinMesh(CComPtr<ID3DXMesh> skinMesh)
{
	D3DVERTEXELEMENT9 declaration[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
		{ 0, 40, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
		D3DDECL_END()
	};

	CComPtr<IDirect3DDevice9> device;
	skinMesh->GetDevice(&device);

	bool simplify = true;

	CComPtr<ID3DXMesh> cloneMesh;
	HRESULT hr = skinMesh->CloneMesh(D3DXMESH_SYSTEMMEM, declaration, device, &cloneMesh);

	std::vector<DWORD> cloneAdjacency(cloneMesh->GetNumFaces() * 3);
	hr = cloneMesh->GenerateAdjacency(0.01f, &cloneAdjacency[0]);

	CComPtr<ID3DXMesh> mesh;
	if (simplify)
	{
		CComPtr<ID3DXMesh> cleanMesh;
		std::vector<DWORD> cleanAdjacency(cloneMesh->GetNumFaces() * 3);
		hr = D3DXCleanMesh(D3DXCLEAN_SIMPLIFICATION, cloneMesh, &cloneAdjacency[0], &cleanMesh, &cleanAdjacency[0], NULL);

		CComPtr<ID3DXMesh> simpleMesh;
		D3DXATTRIBUTEWEIGHTS attributeWeights;
		memset(&attributeWeights, 0, sizeof(attributeWeights));
		attributeWeights.Position = 1.0f;
		//hr = D3DXSimplifyMesh(cleanMesh, &cleanAdjacency[0], &attributeWeights, NULL, (DWORD)(cleanMesh->GetNumFaces() * 0.1f), D3DXMESHSIMP_FACE, &simpleMesh);
		hr = D3DXSimplifyMesh(cleanMesh, &cleanAdjacency[0], &attributeWeights, NULL, 500, D3DXMESHSIMP_FACE, &simpleMesh);

		std::vector<DWORD> simpleAdjacency(simpleMesh->GetNumFaces() * 3);
		hr = simpleMesh->GenerateAdjacency(0.01f, &simpleAdjacency[0]);

		std::vector<DWORD> optimizeAdjacency(simpleMesh->GetNumFaces() * 3);
		hr = simpleMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_COMPACT, &simpleAdjacency[0], &optimizeAdjacency[0], NULL, NULL);

		std::vector<DWORD> meshAdjacency(simpleMesh->GetNumFaces() * 3);
		hr = D3DXCleanMesh(D3DXCLEAN_SIMPLIFICATION, simpleMesh, &optimizeAdjacency[0], &mesh, &meshAdjacency[0], NULL);
	}
	else
	{
		std::vector<DWORD> optimizeAdjacency(cloneMesh->GetNumFaces() * 3);
		hr = cloneMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_COMPACT, &cloneAdjacency[0], &optimizeAdjacency[0], NULL, NULL);

		std::vector<DWORD> meshAdjacency(cloneMesh->GetNumFaces() * 3);
		hr = D3DXCleanMesh(D3DXCLEAN_SIMPLIFICATION, cloneMesh, &optimizeAdjacency[0], &mesh, &meshAdjacency[0], NULL);
	}

	DWORD numVertices = mesh->GetNumVertices();
	DWORD numFaces = mesh->GetNumFaces();

	m_vertices.resize(numVertices);
	m_indices.resize(numFaces * 3);

	CComPtr<IDirect3DVertexBuffer9> vb;
	mesh->GetVertexBuffer(&vb);

	D3DXMATRIX scale;
	D3DXMatrixScaling(&scale, 1.27885294f, 1.12316549f, 1.47023523f);

	TinyVertex* vertices;
	if (SUCCEEDED(vb->Lock(0, 0, (void**)&vertices, 0)))
	{
		for (size_t i = 0; i < numVertices; ++i)
		{ 
			D3DXVECTOR4 result;
			D3DXVec3Transform(&result, &D3DXVECTOR3(vertices[i].x, vertices[i].y, vertices[i].z), &scale);
			vertices[i].x = result.x;
			vertices[i].y = result.y;
			vertices[i].z = result.z;
		}

		memcpy(&m_vertices[0], vertices, sizeof(TinyVertex)* numVertices);

		vb->Unlock();
	}

	CComPtr<IDirect3DIndexBuffer9> ib;
	mesh->GetIndexBuffer(&ib);

	WORD* indices;
	if (SUCCEEDED(ib->Lock(0, 0, (void**)&indices, 0)))
	{
		//memcpy(&m_indices[0], indices, sizeof(WORD)* numFaces * 3);
		std::vector<DWORD> remap(numFaces);
		D3DXOptimizeFaces(indices, numFaces, numVertices, FALSE, &remap[0]);
		for (size_t i = 0; i < numFaces; ++i)
		{
			m_indices[i * 3 + 0] = indices[remap[i] * 3 + 0];
			m_indices[i * 3 + 1] = indices[remap[i] * 3 + 1];
			m_indices[i * 3 + 2] = indices[remap[i] * 3 + 2];
		}

		ib->Unlock();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	TCHAR szTitle[100];
	TCHAR szWindowClass[100];
	LoadString(hInstance, IDS_APP_TITLE, szTitle, 100);
	LoadString(hInstance, IDC_TINYCONVERTER, szWindowClass, 100);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYCONVERTER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_TINYCONVERTER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	//
	CComPtr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS m_pp;
	ZeroMemory(&m_pp, sizeof(D3DPRESENT_PARAMETERS));
	m_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_pp.BackBufferCount = 1;
	m_pp.hDeviceWindow = hWnd;
	m_pp.SwapEffect = D3DSWAPEFFECT_COPY;
	m_pp.EnableAutoDepthStencil = TRUE;
	m_pp.AutoDepthStencilFormat = D3DFMT_D16;
	m_pp.Windowed = TRUE;

	AllocateHierarchy meshAllocator;
	LPD3DXFRAME pFrame;
	CComPtr<ID3DXAnimationController> pAnimationController;

	CComPtr<IDirect3DDevice9> m_device;
	if (SUCCEEDED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_pp, &m_device)))
	{
		HRESULT hr = D3DXLoadMeshHierarchyFromX(L"tiny_4anim.x", 0, m_device, &meshAllocator, NULL, &pFrame, &pAnimationController);

		struct FrameInfo
		{
			FrameInfo() {}
			FrameInfo(LPD3DXFRAME p, LPD3DXFRAME c, D3DXMATRIX w) : parent(p), current(c), world(w) {}

			LPD3DXFRAME parent;
			LPD3DXFRAME current;
			D3DXMATRIX world;
		};

		std::stack<FrameInfo> frameQueue;

		D3DXMATRIX identity;
		D3DXMatrixIdentity(&identity);

		frameQueue.push(FrameInfo(NULL, pFrame, identity));

		while (!frameQueue.empty())
		{
			FrameInfo top = frameQueue.top();
			frameQueue.pop();

			LPD3DXFRAME parent = top.parent;
			LPD3DXFRAME current = top.current;


			if (current->pMeshContainer != NULL 
				&& current->pMeshContainer->MeshData.Type == D3DXMESHTYPE_MESH 
				&& current->pMeshContainer->MeshData.pMesh != NULL)
			{
				ExtractSkinMesh(current->pMeshContainer->MeshData.pMesh);
			}
			
			Bone bone;
			strcpy_s(bone.name, current->Name);
			bone.parent = -1;
			D3DXMatrixDecompose(&bone.scale, &bone.rotation, &bone.translation, &current->TransformationMatrix);

			for (size_t i = 0; i < m_bones.size(); ++i)
			{
				if (parent && strcmp(m_bones[i].name, parent->Name) == 0)
				{
					bone.parent = i;
					break;
				}
			}

			m_bones.push_back(bone);

			if (current->pFrameFirstChild)
				frameQueue.push(FrameInfo(current, current->pFrameFirstChild, current->TransformationMatrix * top.world));

			for (LPD3DXFRAME sibling = current->pFrameSibling; sibling; sibling = sibling->pFrameSibling)
				frameQueue.push(FrameInfo(parent, sibling, top.world));
		}

		UINT numAniationSet = pAnimationController->GetNumAnimationSets();

		m_animationSets.resize(numAniationSet);

		for (UINT i = 0; i < numAniationSet; ++i)
		{
			LPD3DXKEYFRAMEDANIMATIONSET pAnimationSet;
			pAnimationController->GetAnimationSet(i, (LPD3DXANIMATIONSET*)&pAnimationSet);

			LPCSTR pSetName = pAnimationSet->GetName();
			double period = pAnimationSet->GetPeriod();
			UINT numAnimations = pAnimationSet->GetNumAnimations();
			UINT animationStart = m_animations.size();
			UINT animationEnd = animationStart + numAnimations;
			
			AnimationSet& animationSet = m_animationSets[i];
			strcpy_s(animationSet.name, pSetName);
			animationSet.period = period;
			animationSet.animationStart = animationStart;
			animationSet.animationEnd = animationEnd;

			m_animations.resize(animationSet.animationEnd);

			for (UINT j = 0; j < numAnimations; ++j)
			{
				LPCSTR pName;
				pAnimationSet->GetAnimationNameByIndex(j, &pName);

				UINT numTranslation = pAnimationSet->GetNumTranslationKeys(j);
				UINT numRotation = pAnimationSet->GetNumRotationKeys(j);
				UINT numScale = pAnimationSet->GetNumScaleKeys(j);

				Animation& animation = m_animations[animationStart + j];

				strcpy_s(animation.name, pName);
				animation.translationStart = m_translations.size();
				animation.translationEnd = animation.translationStart + numTranslation;
				animation.rotationStart = m_rotations.size();
				animation.rotationEnd = animation.rotationStart + numRotation;
				animation.scaleStart = m_scales.size();
				animation.scaleEnd = animation.scaleStart + numScale;

				m_translations.resize(animation.translationStart + numTranslation);
				m_rotations.resize(animation.rotationStart + numRotation);
				m_scales.resize(animation.scaleStart + numScale);

				pAnimationSet->GetTranslationKeys(j, &m_translations[animation.translationStart]);
				pAnimationSet->GetRotationKeys(j, &m_rotations[animation.rotationStart]);
				pAnimationSet->GetScaleKeys(j, &m_scales[animation.scaleStart]);
			}
		}
	}

	// save tiny.mesh
	FILE* file;
	if (fopen_s(&file, "tiny.tiny", "wb") == 0)
	{
		// bones
		uint numBones = m_bones.size();
		fwrite(&numBones, sizeof(uint), 1, file);
		fwrite(&m_bones[0], sizeof(Bone), numBones, file);

		// meshes
		uint numVertices = m_vertices.size();
		fwrite(&numVertices, sizeof(uint), 1, file);
		fwrite(&m_vertices[0], sizeof(TinyVertex), numVertices, file);

		uint numIndices = m_indices.size();
		fwrite(&numIndices, sizeof(uint), 1, file);
		fwrite(&m_indices[0], sizeof(WORD), numIndices, file);

		uint numSkins = m_skins.size();
		fwrite(&numSkins, sizeof(uint), 1, file);
		fwrite(&m_skins[0], sizeof(Skin), numSkins, file);
		
		//
		uint numTranslations = m_translations.size();
		fwrite(&numTranslations, sizeof(uint), 1, file);
		fwrite(&m_translations[0], sizeof(D3DXKEY_VECTOR3), numTranslations, file);

		uint numRotations = m_rotations.size();
		fwrite(&numRotations, sizeof(uint), 1, file);
		for (auto& rotation : m_rotations) rotation.Value.w = -rotation.Value.w;
		fwrite(&m_rotations[0], sizeof(D3DXKEY_QUATERNION), numRotations, file);

		uint numScales = m_scales.size();
		fwrite(&numScales, sizeof(uint), 1, file);
		fwrite(&m_scales[0], sizeof(D3DXKEY_VECTOR3), numScales, file);

		uint numAnimations = m_animations.size();
		fwrite(&numAnimations, sizeof(uint), 1, file);
		fwrite(&m_animations[0], sizeof(Animation), numAnimations, file);

		uint numAnimationSets = m_animationSets.size();
		fwrite(&numAnimationSets, sizeof(uint), 1, file);
		fwrite(&m_animationSets[0], sizeof(AnimationSet), numAnimationSets, file);

		fclose(file);
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
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
