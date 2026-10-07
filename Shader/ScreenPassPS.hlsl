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
    
    output.Color = Sampled;
    
    float Depth = DepthTexture.Load(int3(pixelCoord, 0)).r;

    const float3 BackgroundColor = float3(0.5f, 0.5f, 0.5f);

    // 아무 메시도 기록되지 않은 픽셀
    if (Depth >= 0.999999f)
    {
        output.Color = float4(BackgroundColor, 1.0f);
    }

    return output;
    
}