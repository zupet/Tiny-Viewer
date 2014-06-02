//
//

#include "stdafx.h"
#include "t_camera.h"

using namespace DirectX;

void TCamera::UpdateView()
{

	
	
	m_vz = XMVector3Normalize(m_lookat - m_eye);
	m_vx = XMVector3Normalize(XMVector3Cross(m_up, m_vz));
	m_vy = XMVector3Normalize(XMVector3Cross(m_vz, m_vx));

	m_view = XMMatrixLookAtLH(m_eye, m_lookat, m_up);
}

void TCamera::UpdateProj()
{
	m_proj = XMMatrixPerspectiveFovLH(m_fov, m_aspect, m_znear, m_zfar);
}