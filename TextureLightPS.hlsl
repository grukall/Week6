#include "Constants.hlsli"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct PS_INPUT
{
    float4 PosH : SV_Position;
    float3 PosW : Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 NormalW : NORMAL;
};

struct PS_OUTPUT
{
    float4 GBufferA : SV_Target0;
    float4 GBufferB : SV_Target1;
    float4 GBufferC : SV_Target2;
};

PS_OUTPUT MainPS(PS_INPUT Input) : SV_Target
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);
    
    //// 하이라이트 색상 보간
    float3 Tint = lerp(float3(1.0f, 1.0f, 1.0f), ColorOverride, ColorOverrideAmount);
    float3 BaseColor = Sampled.rgb * Tint;

    //if (DisableShading > 0.5f)
    //{
    //    return float4(BaseColor, Sampled.a);
    //}
    
    //// 조명 계산 및 양면 음영 보정
    //float3 N = normalize(Input.NormalW);

    Material Mat;
    Mat.DiffAlbedo = diffAlbedo * BaseColor;
    Mat.SpecAlbedo = specAlbedo;
    Mat.Shininess = Shininess;
    
    float3 ToEye = normalize(CamPos - Input.PosW);
    
    //float3 FinalColor = ComputeLight(DirLights, PointLights, SpotLights,
    //                NumDirLights, NumPointLights, NumSpotLights,
    //                Mat, Input.PosW, N, ToEye);
    
    float3 Ambient = Mat.DiffAlbedo * AmbientLight;
    //float3 Result = Ambient + FinalColor;
    //return float4(Result, Input.Color.a);
    
    // Gbuffer
    PS_OUTPUT Output;

    Output.GBufferA = float4(Mat.DiffAlbedo, Mat.Shininess);
    Output.GBufferB = float4(normalize(Input.NormalW) * 0.5f + 0.5f, DisableShading);
    Output.GBufferC = float4(Mat.SpecAlbedo, 0.0f);

    return Output;

}