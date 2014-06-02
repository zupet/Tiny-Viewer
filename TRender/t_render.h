//
//

#if !defined(_T_RENDERER_H_)
#define _T_RENDERER_H_

struct TDeclDesc;
class TDecl;
class TBuffer;
class TShader;
class TTexture;
class TQuery;

enum TMSAA
{
	TMSAA_NONE, TMSAA_MINIMUM, TMSAA_MAXIMUM,
};

class TRenderer
{
public:
	virtual	~TRenderer() {}

	virtual	void Init(HWND hWnd, TMSAA msaa = TMSAA_NONE) = 0;
	
	virtual	bool BeginScene() = 0;
	virtual	void EndScene() = 0;

	virtual	void Clear() = 0;
	virtual	void Present() = 0;
	virtual	void Reset(HWND hWnd) = 0;

	virtual	TDecl* CraeteVertexDecl(TShader* vertexShader, TDeclDesc* decl, int count) = 0;
	virtual	void SetVertexDecl(TDecl* decl) = 0;

	virtual	TBuffer* CreateVertexBuffer(int stride, int count) = 0;
	virtual	void SetVertexBuffer(int stream, TBuffer* buffer) = 0;
	virtual	void UpdateVertexBuffer(TBuffer* buffer, void* data, int len) = 0;

	virtual	TBuffer* CreateIndexBuffer(int size) = 0;
	virtual	void SetIndexBuffer(TBuffer* buffer) = 0;
	virtual	void UpdateIndexBuffer(TBuffer* buffer, void* data, int len) = 0;

	virtual	TBuffer* CreateConstantBuffer(int size) = 0;
	virtual	void SetVertexConstant(int offset, TBuffer* constant) = 0;
	virtual	void SetPixelConstant(int offset, TBuffer* constant) = 0;
	virtual	void SetComputeConstant(int offset, TBuffer* constant) = 0;
	virtual	void UpdateConstantBuffer(TBuffer* buffer, void* data, int len) = 0;

	virtual	TBuffer* CreateStructuredBuffer(int stride, int count) = 0;
	virtual	void SetVertexStructureReadOnly(int offset, TBuffer* constant) = 0;
	virtual	void SetComputeStructureReadOnly(int offset, TBuffer* constant) = 0;
	virtual	void SetComputeStructureReadWrite(int offset, TBuffer* constant) = 0;
	virtual	void UpdateStructuredBuffer(TBuffer* buffer, void* data, int len) = 0;

	virtual	TTexture* CreateTexture(LPCTSTR filename) = 0;
	virtual	void SetTexture(int index, TTexture* texture) = 0;
	virtual	void UpdateTexture(TTexture* texture, void* data, int len) = 0;

	virtual	TShader* CreateVertexShader(LPCTSTR filename);
	virtual	TShader* CreateVertexShader(const void* function, int len) = 0;
	virtual	void SetVertexShader(TShader* vertexShader) = 0;

	virtual	TShader* CreatePixelShader(LPCTSTR filename);
	virtual	TShader* CreatePixelShader(const void* function, int len) = 0;
	virtual	void SetPixelShader(TShader* pixelShader) = 0;

	virtual	TShader* CreateComputeShader(LPCTSTR filename);
	virtual	TShader* CreateComputeShader(const void* function, int len) = 0;
	virtual	void SetComputeShader(TShader* pixelShader) = 0;

	virtual TQuery* CreateQuery() = 0;
	virtual void WaitForQuery(TQuery* query) = 0;

	virtual	void DrawIndexed(int count) = 0;
	virtual	void DrawIndexed(int start, int count) = 0;
	virtual	void DrawIndexedInstanced(int primitiveCount, int instanceCount) = 0;

	virtual	void DrawIndexedLine(int count) = 0;
	virtual void DrawLine(int count) = 0;

	virtual void Dispatch(UINT x, UINT y, UINT z) = 0;

	virtual void CopyResource(TBuffer* buffer, void* dest) = 0;
};

enum TDeclDesc_Type	{ TYPE_POSITION, TYPE_NORMAL, TYPE_TEXTURE, TYPE_BONE, TYPE_WEIGHT, TYPE_COLOR, };
enum TDeclDesc_Format { FORMAT_FLOAT4, FORMAT_FLOAT3, FORMAT_FLOAT2, FORMAT_FLOAT1, FORMAT_HALF2, FORMAT_HALF4, FORMAT_COLOR4, FORMAT_WORD2, FORMAT_WORD4, FORMAT_BYTE4, FORMAT_DWORD };

struct TDeclDesc
{
	TDeclDesc_Type		type;
	TDeclDesc_Format	format;
	int					stream;
	int					index;
	int					offset;
	bool				instance;
};

class TDecl
{
public:
	virtual ~TDecl() {}
};

class TBuffer
{
public:
	virtual ~TBuffer() {}
};

class TShader
{
public:
	virtual ~TShader() {}
};

class TTexture
{
public:
	virtual ~TTexture() {}
};

class TQuery
{
public:
	virtual ~TQuery() {}
};

#endif