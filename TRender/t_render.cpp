//
//

#include "stdafx.h"
#include "t_render.h"
#include <stdio.h>
#include <vector>

/*---------------------------------------------------------------------------*/
TShader* TRenderer::CreateVertexShader(LPCTSTR filename)
{
	TCHAR moduleName[MAX_PATH];
	GetModuleFileName(NULL, moduleName, MAX_PATH);

	TCHAR fullpath[MAX_PATH];
	LPTSTR filePart;
	GetFullPathName(moduleName, MAX_PATH, fullpath, &filePart);

	_tcscpy_s(filePart, MAX_PATH - (filePart - fullpath), filename);

	FILE* file;
	if (_tfopen_s(&file, fullpath, TEXT("rb")) == 0)
	{
		fseek(file, 0, SEEK_END);
		int len = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::vector<char> buffer(len);
		fread(&buffer[0], sizeof(char), len, file);

		fclose(file);

		return CreateVertexShader(&buffer[0], len);
	}
	else
	{
		return NULL;
	}
}

/*---------------------------------------------------------------------------*/
TShader* TRenderer::CreatePixelShader(LPCTSTR filename)
{
	TCHAR moduleName[MAX_PATH];
	GetModuleFileName(NULL, moduleName, MAX_PATH);

	TCHAR fullpath[MAX_PATH];
	LPTSTR filePart;
	GetFullPathName(moduleName, MAX_PATH, fullpath, &filePart);

	_tcscpy_s(filePart, MAX_PATH - (filePart - fullpath), filename);

	FILE* file;
	if (_tfopen_s(&file, fullpath, TEXT("rb")) == 0)
	{
		fseek(file, 0, SEEK_END);
		int len = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::vector<char> buffer(len);
		fread(&buffer[0], sizeof(char), len, file);

		fclose(file);

		return CreatePixelShader(&buffer[0], len);
	}
	else
	{
		return NULL;
	}
}

/*---------------------------------------------------------------------------*/
TShader* TRenderer::CreateComputeShader(LPCTSTR filename)
{
	TCHAR moduleName[MAX_PATH];
	GetModuleFileName(NULL, moduleName, MAX_PATH);

	TCHAR fullpath[MAX_PATH];
	LPTSTR filePart;
	GetFullPathName(moduleName, MAX_PATH, fullpath, &filePart);

	_tcscpy_s(filePart, MAX_PATH - (filePart - fullpath), filename);

	FILE* file;
	if (_tfopen_s(&file, fullpath, TEXT("rb")) == 0)
	{
		fseek(file, 0, SEEK_END);
		int len = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::vector<char> buffer(len);
		fread(&buffer[0], sizeof(char), len, file);

		fclose(file);

		return CreateComputeShader(&buffer[0], len);
	}
	else
	{
		return NULL;
	}
}
