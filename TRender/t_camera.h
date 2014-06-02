//
//

#if !defined(_T_CAMERA_H_)
#define _T_CAMERA_H_

#include <directxmath.h>

class TCamera
{
public:
	virtual ~TCamera() {}

	void SetView(const DirectX::XMVECTOR& eye, const DirectX::XMVECTOR& lookat, const DirectX::XMVECTOR& up) { m_eye = eye; m_lookat = lookat; m_up = up; UpdateView(); }
	void SetEye(const DirectX::XMVECTOR& eye) { m_eye = eye; UpdateView(); }
	const DirectX::XMMATRIX& GetView() { return m_view; }

	const DirectX::XMVECTOR& GetEye() { return m_eye; }
	const DirectX::XMVECTOR& GetViewX() { return m_vx; }
	const DirectX::XMVECTOR& GetViewY() { return m_vy; }
	const DirectX::XMVECTOR& GetViewZ() { return m_vz; }

	void SetProj(float fov, float aspect, float znear, float zfar) { m_fov = fov; m_aspect = aspect; m_znear = znear; m_zfar = zfar; UpdateProj(); }
	void SetAspect(float aspect) { m_aspect = aspect; UpdateProj(); }
	const DirectX::XMMATRIX& GetProj() { return m_proj; }

	const DirectX::XMMATRIX& GetViewProj() { return m_view * m_proj; }

protected:
	void UpdateView();
	void UpdateProj();

	DirectX::XMVECTOR m_eye, m_lookat, m_up;
	DirectX::XMVECTOR m_vx, m_vy, m_vz;
	DirectX::XMMATRIX m_view;

	float m_fov, m_aspect, m_znear, m_zfar;
	DirectX::XMMATRIX m_proj;
};

#endif //_T_CAMERA_H_