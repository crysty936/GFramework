
inline float3x3 ToFloat3x3(float4x4 m) {
    return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}

typedef float4 Quaternion;

float3x3 QuatTo3x3(in Quaternion q)
{
    float3x3 m = float3x3(1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z, 2.0f * q.x * q.y - 2.0f * q.z * q.w, 2.0f * q.x * q.z + 2.0f * q.y * q.w,
                          2.0f * q.x * q.y + 2.0f * q.z * q.w, 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z, 2.0f * q.y * q.z - 2.0f * q.x * q.w,
                          2.0f * q.x * q.z - 2.0f * q.y * q.w, 2.0f * q.y * q.z + 2.0f * q.x * q.w, 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y);
    return transpose(m);
}
