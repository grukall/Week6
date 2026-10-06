#include "Constants.hlsli"
Texture2D SceneTexture : register(t6);
Texture2D DepthTexture : register(t7);
SamplerState SceneSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

PS_OUTPUT MainPS(PS_INPUT Input)
{
    PS_OUTPUT output;
    
    // float4 Sampled = SceneTexture.Sample(SceneSampler, Input.UV); // 기존 색상 출력
    float Depth = DepthTexture.Sample(SceneSampler, Input.UV).r;
    
    float fn = Far - Near;
    float A = Far / fn;
    float B = -Far * Near / fn;
    float z_view = B / (Depth - A);
    
    // 거리 정규화 (나중에 DepthScale 값 받아 조정 필요)
    z_view = (z_view - Near) / 20;
    
    output.Color = z_view ;
    
    return output;
}