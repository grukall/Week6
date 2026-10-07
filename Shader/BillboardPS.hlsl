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

// 빌보드 전용. 조명을 받지 않고 텍스처의 색과 알파를 그대로 내보낸다.
float4 MainPS(PS_INPUT Input) : SV_Target
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);

    float Alpha = Sampled.a;
    clip(Alpha - 0.1f);

    // 하이라이트 색상 보간
    float3 Tint = lerp(float3(1.0f, 1.0f, 1.0f), ColorOverride, ColorOverrideAmount);

    return float4(Sampled.rgb * Tint, Alpha);
}
