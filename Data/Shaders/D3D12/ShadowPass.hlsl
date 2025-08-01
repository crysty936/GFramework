
// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 LocalToClip;
	float4 Test;
	float Padding[43];
};

ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);

float4 VSMain(float4 position : POSITION, float3 VertexNormal : NORMAL, float2 uv : TEXCOORD, float3 tangent : TANGENT, float3 bitangent : BITANGENT) : SV_POSITION
{
    float4 clipPos = mul(position, SceneBuffer.LocalToClip);


    if (clipPos.z < 0)
    {
        // Clamp vertex z to near plane, for directional lights
        clipPos.z = 0.000001f;
    }
    
    return clipPos;
}
