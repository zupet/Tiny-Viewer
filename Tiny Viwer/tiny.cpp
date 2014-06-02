//
//

#include "stdafx.h"
#include "tiny.h"

#include <d3d9.h>
#include <d3dx9.h>

void Tiny::LoadMesh(LPCTSTR meshfile)
{
	FILE* file;

	if (_tfopen_s(&file, meshfile, TEXT("rb")) == 0)
	{
		// bones
		uint numBones;
		fread(&numBones, sizeof(uint), 1, file);
		m_bones.resize(numBones);
		fread(&m_bones[0], sizeof(Bone), numBones, file);

		// meshes
		uint numVertices;
		fread(&numVertices, sizeof(uint), 1, file);
		m_vertices.resize(numVertices);
		fread(&m_vertices[0], sizeof(TinyVertex), numVertices, file);

		uint numIndices;
		fread(&numIndices, sizeof(uint), 1, file);
		m_indices.resize(numIndices);
		fread(&m_indices[0], sizeof(WORD), numIndices, file);

		uint numSkins;
		fread(&numSkins, sizeof(uint), 1, file);
		m_skins.resize(numSkins);
		fread(&m_skins[0], sizeof(Skin), numSkins, file);

		//
		uint numTranslations;
		fread(&numTranslations, sizeof(uint), 1, file);
		m_translations.resize(numTranslations);
		fread(&m_translations[0], sizeof(D3DXKEY_VECTOR3), numTranslations, file);

		uint numRotations;
		fread(&numRotations, sizeof(uint), 1, file);
		m_rotations.resize(numRotations);
		fread(&m_rotations[0], sizeof(D3DXKEY_QUATERNION), numRotations, file);

		uint numScales;
		fread(&numScales, sizeof(uint), 1, file);
		m_scales.resize(numScales);
		fread(&m_scales[0], sizeof(D3DXKEY_VECTOR3), numScales, file);

		uint numAnimations;
		fread(&numAnimations, sizeof(uint), 1, file);
		m_animations.resize(numAnimations);
		fread(&m_animations[0], sizeof(Animation), numAnimations, file);

		uint numAnimationSets;
		fread(&numAnimationSets, sizeof(uint), 1, file);
		m_animationSets.resize(numAnimationSets);
		fread(&m_animationSets[0], sizeof(AnimationSet), numAnimationSets, file);

		for (size_t i = 0; i < numBones; ++i)
		{
			const Bone& bone = m_bones[i];
			m_nameToBone.insert(std::map<std::string, int>::value_type(bone.name, i));
		}

		for (size_t i = 0; i < numSkins; ++i)
		{
			const Skin& skin = m_skins[i];
			size_t index = m_nameToBone[skin.name];
			m_skinToBone.push_back(index);
		}

		fclose(file);
	}
}
