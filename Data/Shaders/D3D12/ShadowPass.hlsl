
// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 LocalToWorld;
	uint Padding[48];
};

ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);

float4 VSMain(float4 position : POSITION, float3 VertexNormal : NORMAL, float2 uv : TEXCOORD, float3 tangent : TANGENT, float3 bitangent : BITANGENT) : SV_POSITION
{
    const float4 clipPos = mul(position, SceneBuffer.LocalToWorld);
    
    return clipPos;
}
