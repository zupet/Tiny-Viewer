float3 Vec3Mul(float3 v, float4 r)
{
	float3 t = 2.0 * cross(r.xyz, v.xyz);
	return v.xyz + r.w * t + cross(r.xyz, t);
}

float4 Vec4Mul(float4 v, float4 r)
{
	float3 t = 2.0 * cross(r.xyz, v.xyz);
	return float4(v.xyz + r.w * t + cross(r.xyz, t), 1);
}

float4 QuatMul(float4 l, float4 r)
{
	float4 rot;
	rot.w = l.w*r.w - l.x*r.x - l.y*r.y - l.z*r.z;
	rot.x = l.w*r.x + l.x*r.w + l.y*r.z - l.z*r.y;
	rot.y = l.w*r.y - l.x*r.z + l.y*r.w + l.z*r.x;
	rot.z = l.w*r.z + l.x*r.y - l.y*r.x + l.z*r.w;
	return rot;
}

struct Transform
{
	float3 pos;
	float4 rot;
};

Transform TransformMul(Transform parent, Transform child)
{
	Transform result;
	result.pos = Vec3Mul(child.pos, parent.rot) + parent.pos;
	result.rot = QuatMul(parent.rot, child.rot);
	return result;
}

float4 Vec4Transform(float4 v, float3 p, float4 r)
{
	float4 result = Vec4Mul(v, r);
	result.xyz = result.xyz + p;
	return result;
}

float3 Vec3Transform(float3 v, float3 p, float4 r)
{
	float3 result = Vec3Mul(v, r);
	return result + p;
}

float4x4 TransformToMatrix(Transform transform)
{
	float4x4 mat;

	float x = transform.rot.x;
	float y = transform.rot.y;
	float z = transform.rot.z;
	float w = transform.rot.w;

	mat[0][0] = 1.0 - 2.0 * y * y - 2.0 * z * z;
	mat[0][1] = 2.0 * x * y + 2.0 * z * w;
	mat[0][2] = 2.0 * x * z - 2.0 * y * w;
	mat[0][3] = 0.0;
	mat[1][0] = 2.0 * x * y - 2.0 * z * w;
	mat[1][1] = 1.0 - 2.0 * x * x - 2.0 * z * z;
	mat[1][2] = 2.0 * y * z + 2.0 * x * w;
	mat[1][3] = 0.0;
	mat[2][0] = 2.0 * x * z + 2.0 * y * w;
	mat[2][1] = 2.0 * y * z - 2.0 * x * w;
	mat[2][2] = 1.0 - 2.0 * x * x - 2.0 * y * y;
	mat[2][3] = 0.0;
	mat[3][0] = transform.pos.x;
	mat[3][1] = transform.pos.y;
	mat[3][2] = transform.pos.z;
	mat[3][3] = 1.0;

	return mat;
}
