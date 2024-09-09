
inline float3x3 ToFloat3x3(float4x4 m) {
    return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}
