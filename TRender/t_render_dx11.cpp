//
//

#include "stdafx.h"
#include "t_render_dx11.h"

#include <vector>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "d3d11.lib")


#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

HRESULT CreateWICTextureFromMemory(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_bytecount_(wicDataSize) const uint8_t* wicData,
	_In_ size_t wicDataSize,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize = 0
	);

HRESULT CreateWICTextureFromFile(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_z_ const wchar_t* szFileName,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize = 0
	);

void TRendererDX11::Init(HWND hWnd, TMSAA msaa)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	UINT width = clientRect.right - clientRect.left;
	UINT height = clientRect.bottom - clientRect.top;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));
	SwapChainDesc.BufferDesc.Width = width;
	SwapChainDesc.BufferDesc.Height = height;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.OutputWindow = hWnd;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = 0;

	D3D_FEATURE_LEVEL FeatureLevel[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	UINT flags = D3D11_CREATE_DEVICE_DEBUG;
	//UINT flags = 0;
	if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, FeatureLevel, 6, D3D11_SDK_VERSION, &SwapChainDesc, &m_swapChain, &m_device, NULL, &m_context)))
	{
		SetDefaultRenderTargets(width, height);

		// Set Default State
		CD3D11_DEFAULT d3dDefault;

		CD3D11_RASTERIZER_DESC rasterizerDesc(d3dDefault);
		if (SUCCEEDED(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState)))
		{
			m_context->RSSetState(m_rasterizerState);
		}

		CD3D11_SAMPLER_DESC samplerDesc(d3dDefault);
		if (SUCCEEDED(m_device->CreateSamplerState(&samplerDesc, &m_samplerState)))
		{
			m_context->PSSetSamplers(0, 1, &m_samplerState.p);
		}

		CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(d3dDefault);
		if (SUCCEEDED(m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState)))
		{
			m_context->OMSetDepthStencilState(m_depthStencilState, 0);
		}

		CD3D11_BLEND_DESC blendDesc(d3dDefault);
		if (SUCCEEDED(m_device->CreateBlendState(&blendDesc, &m_blendState)))
		{
			float blendFactor[4] = { 1, 1, 1, 1 };
			m_context->OMSetBlendState(m_blendState, blendFactor, 0xFFFFFFFF);
		}
	}
}

void TRendererDX11::SetDefaultRenderTargets(UINT width, UINT height)
{
	CComPtr<ID3D11Texture2D> defaultTarget;
	if (SUCCEEDED(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&defaultTarget)))
	{
		if (SUCCEEDED(m_device->CreateRenderTargetView(defaultTarget, NULL, &m_defaultTargetView)))
		{
		}
	}

	CComPtr<ID3D11Texture2D> depthStencil;
	CD3D11_TEXTURE2D_DESC descDepth(DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
	if (SUCCEEDED(m_device->CreateTexture2D(&descDepth, NULL, &depthStencil)))
	{
		if (SUCCEEDED(m_device->CreateDepthStencilView(depthStencil, NULL, &m_depthStencilView)))
		{
		}
	}

	m_context->OMSetRenderTargets(1, &m_defaultTargetView.p, m_depthStencilView);

	CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)width, (float)height);
	m_context->RSSetViewports(1, &viewport);
}

bool TRendererDX11::BeginScene()
{
	return true;
}

void TRendererDX11::EndScene()
{
}

void TRendererDX11::Clear()
{
	float color[4] = { 0, 0, 0, 0 };
	m_context->ClearRenderTargetView(m_defaultTargetView, color);
	m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void TRendererDX11::Present()
{
	m_swapChain->Present(0, 0);
}

void TRendererDX11::Reset(HWND hWnd)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	UINT width = clientRect.right - clientRect.left;
	UINT height = clientRect.bottom - clientRect.top;

	HRESULT hr;

	m_defaultTargetView = NULL;
	m_depthStencilView = NULL;
	m_context->OMSetRenderTargets(1, &m_defaultTargetView.p, m_depthStencilView);

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	hr = m_swapChain->GetDesc(&swapChainDesc);

	hr = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	hr = m_swapChain->GetDesc(&swapChainDesc);

	SetDefaultRenderTargets(width, height);
}

const char* DescTypeToSemanticName(TDeclDesc_Type type)
{
	switch (type)
	{
	case TYPE_POSITION:
		return "POSITION";
	case TYPE_NORMAL:
		return "NORMAL";
	case TYPE_TEXTURE:
		return "TEXCOORD";
		//return "TEXTURE";
	case TYPE_BONE:
		return "BLENDINDICES";
		//return "BONE";
	case TYPE_WEIGHT:
		return "BLENDWEIGHT";
		//return "WEIGHT";
	case TYPE_COLOR:
		return "COLOR";
	default:
		__debugbreak();
		return "";
	}
}

DXGI_FORMAT DescFormatToFormat(TDeclDesc_Format format)
{
	switch (format)
	{
	case FORMAT_FLOAT4:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case FORMAT_FLOAT3:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case FORMAT_FLOAT2:
		return DXGI_FORMAT_R32G32_FLOAT;
	case FORMAT_FLOAT1:
		return DXGI_FORMAT_R32_FLOAT;
	case FORMAT_HALF2:
		return DXGI_FORMAT_R16G16_FLOAT;
	case FORMAT_HALF4:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case FORMAT_COLOR4:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case FORMAT_WORD2:
		return DXGI_FORMAT_R16G16_UINT;
	case FORMAT_WORD4:
		return DXGI_FORMAT_R16G16B16A16_UINT;
	case FORMAT_BYTE4:
		return DXGI_FORMAT_R8G8B8A8_UINT;
	case FORMAT_DWORD:
		return DXGI_FORMAT_R8G8B8A8_UINT;
	default:
		__debugbreak();
		return DXGI_FORMAT_UNKNOWN;
	}
}

TDecl* TRendererDX11::CraeteVertexDecl(TShader* shader, TDeclDesc* decl, int count)
{
	auto& blob = ((TVertexShaderDX11*)shader)->m_blob;

	std::vector<D3D11_INPUT_ELEMENT_DESC> declList(count);

	for (int i = 0; i < count; ++i)
	{
		declList[i].SemanticName = DescTypeToSemanticName(decl[i].type);
		declList[i].SemanticIndex = (UINT)decl[i].index;
		declList[i].Format = DescFormatToFormat(decl[i].format);
		declList[i].InputSlot = (UINT)decl[i].stream;
		declList[i].AlignedByteOffset = decl[i].offset;
		declList[i].InputSlotClass = decl[i].instance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
		declList[i].InstanceDataStepRate = 0;
	}

	CComPtr<ID3D11InputLayout> inputLayout;
	if (SUCCEEDED(m_device->CreateInputLayout(&declList[0], count, &blob[0], blob.size(), &inputLayout)))
	{
		return new TVertexDeclDX11(inputLayout);
	}
	return NULL;
}

void TRendererDX11::SetVertexDecl(TDecl* decl)
{
	m_context->IASetInputLayout(((TVertexDeclDX11*)decl)->m_inputLayout);
}

TBuffer* TRendererDX11::CreateVertexBuffer(int stride, int count)
{
	CD3D11_BUFFER_DESC desc(count * stride, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DEFAULT, 0, 0, stride);

	CComPtr<ID3D11Buffer> vertexBuffer;
	if (SUCCEEDED(m_device->CreateBuffer(&desc, NULL, &vertexBuffer)))
	{
		return new TBufferDX11(desc, vertexBuffer);
	}
	return NULL;
}

void TRendererDX11::SetVertexBuffer(int stream, TBuffer* buffer)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;
	UINT stride = ((TBufferDX11*)buffer)->m_desc.StructureByteStride;
	UINT offset = 0;

	m_context->IASetVertexBuffers(stream, 1, &buffers, &stride, &offset);
}

void TRendererDX11::UpdateVertexBuffer(TBuffer* buffer, void* data, int len)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	D3D11_BOX box = { 0, 0, 0, len, 1, 1 };

	m_context->UpdateSubresource(buffers, 0, &box, data, 0, 0);
}

TBuffer* TRendererDX11::CreateIndexBuffer(int size)
{
	CD3D11_BUFFER_DESC desc(size, D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_DEFAULT, 0, 0, sizeof(WORD));

	CComPtr<ID3D11Buffer> indexBuffer;
	if (SUCCEEDED(m_device->CreateBuffer(&desc, NULL, &indexBuffer)))
	{
		return new TBufferDX11(desc, indexBuffer);
	}
	return NULL;
}

void TRendererDX11::SetIndexBuffer(TBuffer* buffer)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	m_context->IASetIndexBuffer(buffers, DXGI_FORMAT_R16_UINT, 0);
}

void TRendererDX11::UpdateIndexBuffer(TBuffer* buffer, void* data, int len)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	D3D11_BOX box = { 0, 0, 0, len, 1, 1 };

	m_context->UpdateSubresource(buffers, 0, &box, data, 0, 0);
}

TBuffer* TRendererDX11::CreateConstantBuffer(int size)
{
	size = (size + 15) & 0xFFFFFFF0;

	CD3D11_BUFFER_DESC desc(size, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT);

	CComPtr<ID3D11Buffer> constantBuffer;
	if (SUCCEEDED(m_device->CreateBuffer(&desc, NULL, &constantBuffer)))
	{
		return new TBufferDX11(desc, constantBuffer);
	}
	return NULL;
}

void TRendererDX11::SetVertexConstant(int offset, TBuffer* buffer)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	m_context->VSSetConstantBuffers(offset, 1, &buffers);
}

void TRendererDX11::SetPixelConstant(int offset, TBuffer* buffer)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	m_context->PSSetConstantBuffers(offset, 1, &buffers);
}

void TRendererDX11::SetComputeConstant(int offset, TBuffer* buffer)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	m_context->CSSetConstantBuffers(offset, 1, &buffers);
}

void TRendererDX11::UpdateConstantBuffer(TBuffer* buffer, void* data, int len)
{
	ID3D11Buffer* buffers = ((TBufferDX11*)buffer)->m_buffer;

	m_context->UpdateSubresource(buffers, 0, NULL, data, 0, 0);
}

TBuffer* TRendererDX11::CreateStructuredBuffer(int stride, int count)
{
	CD3D11_BUFFER_DESC desc(stride * count, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, stride);

	CComPtr<ID3D11Buffer> structuredBuffer;
	if (SUCCEEDED(m_device->CreateBuffer(&desc, NULL, &structuredBuffer)))
	{
		CComPtr<ID3D11ShaderResourceView> resourceView = NULL;
		CD3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc(structuredBuffer, DXGI_FORMAT_UNKNOWN, 0, desc.ByteWidth / desc.StructureByteStride);
		m_device->CreateShaderResourceView(structuredBuffer, &resourceDesc, &resourceView);

		CComPtr<ID3D11UnorderedAccessView> unorderedView = NULL;
		CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedDesc(structuredBuffer, DXGI_FORMAT_UNKNOWN, 0, desc.ByteWidth / desc.StructureByteStride, 0);
		m_device->CreateUnorderedAccessView(structuredBuffer, &unorderedDesc, &unorderedView);

		return new TStructuredBufferDX11(desc, structuredBuffer, resourceView, unorderedView);
	}
	return NULL;
}

void TRendererDX11::SetVertexStructureReadOnly(int offset, TBuffer* constant)
{
	ID3D11ShaderResourceView* resourceView = constant ? ((TStructuredBufferDX11*)constant)->m_resourceView : NULL;
	m_context->VSSetShaderResources(offset, 1, &resourceView);
}

void TRendererDX11::SetComputeStructureReadOnly(int offset, TBuffer* constant)
{
	ID3D11ShaderResourceView* resourceView = constant ? ((TStructuredBufferDX11*)constant)->m_resourceView : NULL;

	m_context->CSSetShaderResources(offset, 1, &resourceView);
}

void TRendererDX11::SetComputeStructureReadWrite(int offset, TBuffer* constant)
{
	ID3D11UnorderedAccessView* unorderedView = constant ? ((TStructuredBufferDX11*)constant)->m_unorderedView : NULL;
	UINT count = 0;

	m_context->CSSetUnorderedAccessViews(offset, 1, &unorderedView, &count);
}

void TRendererDX11::UpdateStructuredBuffer(TBuffer* constant, void* data, int len)
{
	ID3D11Buffer* buffers = ((TStructuredBufferDX11*)constant)->m_buffer;

	D3D11_BOX box = { 0, 0, 0, len, 1, 1 };
	
	m_context->UpdateSubresource(buffers, 0, &box, data, 0, 0);
}

TTexture* TRendererDX11::CreateTexture(LPCTSTR filename)
{
	CComPtr<ID3D11Resource> texture;
	CComPtr<ID3D11ShaderResourceView> resourceView;
	if (SUCCEEDED(CreateWICTextureFromFile(m_device, m_context, filename, &texture, &resourceView)))
	{
		return new TTextureDX11(texture, resourceView);
	}
	return NULL;	
}

void TRendererDX11::SetTexture(int index, TTexture* texture)
{
	ID3D11ShaderResourceView* resourceViews = texture ? ((TTextureDX11*)texture)->m_resourceView : NULL;

	m_context->PSSetShaderResources(index, 1, &resourceViews);
}

void TRendererDX11::UpdateTexture(TTexture* texture, void* data, int len)
{
	ID3D11Resource* resource = ((TTextureDX11*)texture)->m_texture;

	m_context->UpdateSubresource(resource, 0, NULL, data, 0, 0);
}

TShader* TRendererDX11::CreateVertexShader(const void* function, int len)
{
	CComPtr<ID3D11VertexShader> vertexShader;
	if (SUCCEEDED(m_device->CreateVertexShader(function, len, NULL, &vertexShader)))
	{
		return new TVertexShaderDX11(std::vector<char>((char*)function, (char*)function + len), vertexShader);
	}
	else
	{
		return NULL;
	}
}

void TRendererDX11::SetVertexShader(TShader* vertexShader)
{
	m_context->VSSetShader(((TVertexShaderDX11*)vertexShader)->m_vertexShader, NULL, 0);
}

TShader* TRendererDX11::CreatePixelShader(const void* function, int len)
{
	CComPtr<ID3D11PixelShader> pixelShader;
	if (SUCCEEDED(m_device->CreatePixelShader(function, len, NULL, &pixelShader)))
	{
		return new TPixelShaderDX11(pixelShader);
	}
	else
	{
		return NULL;
	}
}

void TRendererDX11::SetPixelShader(TShader* pixelShader)
{
	m_context->PSSetShader(((TPixelShaderDX11*)pixelShader)->m_pixelShader, NULL, 0);
}

TShader* TRendererDX11::CreateComputeShader(const void* function, int len)
{
	CComPtr<ID3D11ComputeShader> computeShader;
	if (SUCCEEDED(m_device->CreateComputeShader(function, len, NULL, &computeShader)))
	{
		return new TComputeShaderDX11(computeShader);
	}
	else
	{
		return NULL;
	}
}

void TRendererDX11::SetComputeShader(TShader* computeShader)
{
	m_context->CSSetShader(((TComputeShaderDX11*)computeShader)->m_computeShader, NULL, 0);
}

TQuery* TRendererDX11::CreateQuery()
{
	CComPtr<ID3D11Query> query;
	CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_EVENT);
	if (SUCCEEDED(m_device->CreateQuery(&queryDesc, &query)))
	{
		return new TQueryDX11(query);
	}
	else
	{
		return NULL;
	}
}

void TRendererDX11::WaitForQuery(TQuery* query)
{
	m_context->End(((TQueryDX11*)query)->m_query);
	while (m_context->GetData(((TQueryDX11*)query)->m_query, NULL, 0, 0) == S_FALSE) {}
}

void TRendererDX11::DrawIndexed(int count)
{
	m_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->DrawIndexed(count * 3, 0, 0);
}

void TRendererDX11::DrawIndexed(int offset, int count)
{
	m_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->DrawIndexed(count * 3, offset, 0);
}

void TRendererDX11::DrawIndexedInstanced(int primitiveCount, int instanceCount)
{
	m_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->DrawIndexedInstanced(primitiveCount * 3, instanceCount, 0, 0, 0);
}

void TRendererDX11::DrawIndexedLine(int count)
{
	m_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	m_context->DrawIndexed(count * 2, 0, 0);
}

void TRendererDX11::DrawLine(int count)
{
	m_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	m_context->Draw(count * 2, 0);
}

void TRendererDX11::Dispatch(UINT x, UINT y, UINT z)
{
	m_context->Dispatch(x, y, z);

}

void TRendererDX11::CopyResource(TBuffer* buffer, void* dest)
{
	D3D11_BUFFER_DESC desc = ((TBufferDX11*)buffer)->m_desc;
	desc.BindFlags = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	CComPtr<ID3D11Buffer> pBuffer;
	m_device->CreateBuffer(&desc, NULL, &pBuffer);

	m_context->CopyResource(pBuffer, ((TBufferDX11*)buffer)->m_buffer);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_context->Map(pBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);

	memcpy(dest, mappedResource.pData, desc.ByteWidth);

	m_context->Unmap(pBuffer, 0);
	
	pBuffer = NULL;
}