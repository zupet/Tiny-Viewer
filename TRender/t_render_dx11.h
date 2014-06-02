//
//

#pragma once

#include "t_render.h"
#include <atlbase.h>
#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11SamplerState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;

class TRendererDX11 : public TRenderer
{
public:
	void Init(HWND hWnd, TMSAA msaa = TMSAA_NONE);

	void SetDefaultRenderTargets(UINT width, UINT height);
	
	bool BeginScene();
	void EndScene();

	void Clear();
	void Present();
	void Reset(HWND hWnd);

	TDecl* CraeteVertexDecl(TShader* shader, TDeclDesc* decl, int count);
	void SetVertexDecl(TDecl* decl);

	TBuffer* CreateVertexBuffer(int stride, int count);
	void SetVertexBuffer(int stream, TBuffer* buffer);
	void UpdateVertexBuffer(TBuffer* buffer, void* data, int len);

	TBuffer* CreateIndexBuffer(int size);
	void SetIndexBuffer(TBuffer* buffer);
	void UpdateIndexBuffer(TBuffer* buffer, void* data, int len);

	TBuffer* CreateConstantBuffer(int size);
	void SetVertexConstant(int offset, TBuffer* constant);
	void SetPixelConstant(int offset, TBuffer* constant);
	void SetComputeConstant(int offset, TBuffer* constant);
	void UpdateConstantBuffer(TBuffer* buffer, void* data, int len);

	TBuffer* CreateStructuredBuffer(int stride, int count);
	void SetVertexStructureReadOnly(int offset, TBuffer* constant);
	void SetComputeStructureReadOnly(int offset, TBuffer* constant);
	void SetComputeStructureReadWrite(int offset, TBuffer* constant);
	void UpdateStructuredBuffer(TBuffer* buffer, void* data, int len);

	TTexture* CreateTexture(LPCTSTR filename);
	void SetTexture(int index, TTexture* texture);
	void UpdateTexture(TTexture* texture, void* data, int len);

	TShader* CreateVertexShader(const void* function, int len);
	void SetVertexShader(TShader* vertexShader);

	TShader* CreatePixelShader(const void* function, int len);
	void SetPixelShader(TShader* pixelShader);

	TShader* CreateComputeShader(const void* function, int len);
	void SetComputeShader(TShader* pixelShader);

	TQuery* CreateQuery();
	void WaitForQuery(TQuery* query);

	void DrawIndexed(int count);
	void DrawIndexed(int offset, int count);
	void DrawIndexedInstanced(int primitiveCount, int instanceCount);

	void DrawIndexedLine(int count);
	void DrawLine(int count);

	void Dispatch(UINT x, UINT y, UINT z);

	void CopyResource(TBuffer* buffer, void* dest);

protected:
	CComPtr<ID3D11Device>				m_device;
	CComPtr<ID3D11DeviceContext>		m_context;

	CComPtr<IDXGISwapChain>				m_swapChain;

	CComPtr<ID3D11RenderTargetView>		m_defaultTargetView;

	//CComPtr<ID3D11Texture2D>			m_renderTarget[4];
	//CComPtr<ID3D11RenderTargetView>		m_renderTargetView[4];
	//CComPtr<ID3D11ShaderResourceView>	m_renderTargetResource[4];

	CComPtr<ID3D11DepthStencilView>		m_depthStencilView;
	
	CComPtr<ID3D11InputLayout>			m_ilSimple;
	CComPtr<ID3D11VertexShader>			m_vsSimple;
	CComPtr<ID3D11PixelShader>			m_psSimple;

	CComPtr<ID3D11Buffer>				m_vertexBffer;
	
	
	CComPtr<ID3D11SamplerState>			m_samplerState;
	CComPtr<ID3D11RasterizerState>		m_rasterizerState;
	CComPtr<ID3D11DepthStencilState>	m_depthStencilState;
	CComPtr<ID3D11BlendState>			m_blendState;
};

class TVertexDeclDX11 : public TDecl
{
public:
	TVertexDeclDX11(ID3D11InputLayout* inputLayout) :m_inputLayout(inputLayout) {}

	CComPtr<ID3D11InputLayout> m_inputLayout;
};

class TVertexShaderDX11 : public TShader
{
public:
	TVertexShaderDX11(std::vector<char>& blob, ID3D11VertexShader* vertexShader) :m_blob(blob), m_vertexShader(vertexShader) {}

	std::vector<char> m_blob;
	CComPtr<ID3D11VertexShader> m_vertexShader;
};

class TPixelShaderDX11 : public TShader
{
public:
	TPixelShaderDX11(ID3D11PixelShader* pixelShader) :m_pixelShader(pixelShader) {}

	CComPtr<ID3D11PixelShader> m_pixelShader;
};

class TComputeShaderDX11 : public TShader
{
public:
	TComputeShaderDX11(ID3D11ComputeShader* computeShader) :m_computeShader(computeShader) {}

	CComPtr<ID3D11ComputeShader> m_computeShader;
};

class TBufferDX11 : public TBuffer
{
public:
	TBufferDX11(const CD3D11_BUFFER_DESC& desc, ID3D11Buffer* buffer) :m_desc(desc), m_buffer(buffer) {}

	CD3D11_BUFFER_DESC		m_desc;
	CComPtr<ID3D11Buffer>	m_buffer;
};

class TStructuredBufferDX11 : public TBufferDX11
{
public:
	TStructuredBufferDX11(const CD3D11_BUFFER_DESC& desc, ID3D11Buffer* buffer, ID3D11ShaderResourceView* resource, ID3D11UnorderedAccessView* unordered)
		: TBufferDX11(desc, buffer)
		, m_resourceView(resource)
		, m_unorderedView(unordered)
	{}

	CComPtr<ID3D11ShaderResourceView> m_resourceView;
	CComPtr<ID3D11UnorderedAccessView> m_unorderedView;
};

class TTextureDX11 : public TTexture
{
public:
	TTextureDX11(ID3D11Resource* texture, ID3D11ShaderResourceView* resourceView)
		:m_texture(texture), m_resourceView(resourceView) {}

	CComPtr<ID3D11Resource> m_texture;
	CComPtr<ID3D11ShaderResourceView> m_resourceView;
};

class TQueryDX11 : public TQuery
{
public:
	TQueryDX11(ID3D11Query* query)
		:m_query(query) {}

	CComPtr<ID3D11Query> m_query;
};

