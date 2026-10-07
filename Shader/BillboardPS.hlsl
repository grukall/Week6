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

// 빌보드 전용. 조명을 받지 않도록 GBufferB.a(DisableShading)에 1을 기록해
// DeferredLighting 패스에서 텍스처 색을 그대로 내보내게 한다.
PS_OUTPUT MainPS(PS_INPUT Input)
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);

    clip(Sampled.a - 0.1f);

    // 하이라이트 색상 보간
    float3 Tint = lerp(float3(1.0f, 1.0f, 1.0f), ColorOverride, ColorOverrideAmount);

    // GBuffer에는 반투명을 표현할 수 없으므로 알파는 1로 덮어쓴다.
    PS_OUTPUT Output;
    Output.GBufferA = float4(Sampled.rgb * Tint, 1.0f);
    Output.GBufferB = float4(0.5f, 0.5f, 1.0f, 1.0f);
    Output.GBufferC = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return Output;
}
