#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 VertexNormalWS : VERTEX_NORMAL;
    float3 VertexTangentWS : VERTEX_TANGENT;
    float3 VertexBitangentWS : VERTEX_BITANGENT;
    float3x3 TangentToWorld : TANGENT_TO_WORLD;
    float4 clipSpacePos : CLIP_POS;
};

struct PSOutput
{
    float4 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Roughness : SV_TARGET2;
};

struct ShaderMaterial
{
	uint AlbedoMapIndex;
	uint NormalMapIndex;
	uint MRMapIndex;
};

StructuredBuffer<ShaderMaterial> MaterialsBuffer : register(t0, space100);

// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 Model;
    float4x4 Projection;
    float4x4 View;
    float4x4 LocalToWorldRotationOnly;
};

struct MatIndexBuffer
{
    uint ShaderMaterialIdx;
};

// 256 byte aligned
ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);
ConstantBuffer<MatIndexBuffer> MatIndex : register(b0);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float3 VertexNormal : NORMAL, float2 uv : TEXCOORD, float3 tangent : TANGENT, float3 bitangent : BITANGENT)
{
    float4x4 mvp = mul(SceneBuffer.Model, SceneBuffer.View);
    mvp = mul(mvp, SceneBuffer.Projection);
    const float4 clipPos = mul(position, mvp);
    
    const float3x3 LocalToWorldRotationOnly3x3 = ToFloat3x3(SceneBuffer.LocalToWorldRotationOnly);

    float3 n = normalize(VertexNormal);
    float3 b = normalize(bitangent);

    // Fix tangents from gltf model being broken
    float3 t = normalize(cross(b, n));

    b = normalize(cross(n, t));

    float3 vertexNormalWS = normalize(mul(n, LocalToWorldRotationOnly3x3)).xyz;
    float3 tangentWS = normalize(mul(t, LocalToWorldRotationOnly3x3)).xyz;
    float3 bitangentWS = normalize(mul(b, LocalToWorldRotationOnly3x3)).xyz;

    float3x3 tangentToLocal = 0;
    tangentToLocal[0] = t;
    tangentToLocal[1] = b;
    tangentToLocal[2] = n;

    float3x3 tangentToWorld = mul(tangentToLocal, LocalToWorldRotationOnly3x3);

	// Gram - Schmidt process
//     float3 tangentWS = normalize(mul(tangent, LocalToWorldRotationOnly3x3));
//     tangentWS = normalize(tangentWS - dot(tangentWS, vertexNormalWS) * vertexNormalWS);
//     float3 bitangentWS = cross(vertexNormalWS, tangentWS);
//     float3x3 tangentToWorld = float3x3(tangentWS, bitangentWS, vertexNormalWS);
//     

    PSInput result;

    result.VertexNormalWS = vertexNormalWS;

    result.uv = uv;
    // Inverse y to fix opengl uv space
    result.uv.y = 1 - uv.y;

    result.position = clipPos;
    result.clipSpacePos = clipPos;

    result.TangentToWorld = tangentToWorld;

    result.VertexTangentWS = tangentWS;
    result.VertexBitangentWS = bitangentWS;

    return result;
}

PSOutput PSMain(PSInput input)
{
    float2 uv = input.uv;

    float3 vertexNormalWS = normalize(input.VertexNormalWS);
    float3 tangentWS = normalize(input.VertexTangentWS);
    float3 bitangentWS = normalize(input.VertexBitangentWS);

    //float3x3 perPixelTangentToWorld = float3x3(tangentWS, bitangentWS, normalWS);

    ShaderMaterial mat = MaterialsBuffer[MatIndex.ShaderMaterialIdx];

    //Texture2D AlbedoMap = Tex2DTable[NonUniformResourceIndex(mat.AlbedoMapIndex)];

    Texture2D AlbedoMap = Tex2DTable[mat.AlbedoMapIndex];
    Texture2D NormalMap = Tex2DTable[mat.NormalMapIndex];
    Texture2D MetallicRoughnessMap = Tex2DTable[mat.MRMapIndex];

    PSOutput output;
    float4 albedo = AlbedoMap.Sample(g_sampler, uv);
    if (albedo.a == 0.f)
    {
        discard;
    }

    output.Albedo = albedo;
    output.Roughness = MetallicRoughnessMap.Sample(g_sampler, uv);

    const float3 normalTS = NormalMap.Sample(g_sampler, uv).xyz * 2.f - 1.f;
    const float3 wsNormal = mul(normalTS, input.TangentToWorld);

    output.Normal = float4(wsNormal / 2.0 + 0.5, 1);

    //output.Albedo = float4(tangentWS / 2.0 + 0.5, 1);

    // Visualize Clip space coords
    // NOTE: SV_Position is transformed to screen coordinates, (0,0) to (viewport.Width, viewport.Height).
    //float2 homPos = input.clipSpacePos.xy / input.clipSpacePos.w;
    //float2 zeroToOnePos = (homPos / 2.f) + 0.5f;    
    //output.Albedo = float4(zeroToOnePos, 0.f, 1.f);


    return output;
}
