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
    
    const int2 pixelCoord = int2(Input.Position.xy);
    float4 Sampled = SceneTexture.Load(int3(pixelCoord, 0)); // 기존 색상 출력
    
    float Depth = DepthTexture.Load(int3(pixelCoord, 0)).r;
    float4 FogColor = float4(0.0, 1.0, 0.0, 1.0);
    
    float fn = Far - Near;
    float A = Far / fn;
    float B = -Far * Near / fn;
    float z_view = B / (Depth - A);
    
    float fogFactor = 1 - exp(-z_view * 0.1f); // 깊이에 따른 안개 효과 계산
    
    
    
    output.Color = lerp(Sampled, FogColor, fogFactor);
    
    return output;
}