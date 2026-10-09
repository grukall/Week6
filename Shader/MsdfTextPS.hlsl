#include "Constants.hlsli"

Texture2D msdTexture : register(t0);
SamplerState msdSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

// 새 채널의 중간값을 구하는 함수
float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

struct PS_OUTPUT
{
    float4 GBufferA : SV_Target0;
    float4 GBufferB : SV_Target1;
    float4 GBufferC : SV_Target2;
};

// 텍스트는 조명을 받지 않도록 GBufferB.a(DisableShading)에 1을 기록한다.
// 오버레이처럼 렌더 타깃이 하나만 바인딩된 경우엔 SV_Target0만 쓰이므로 그대로 동작한다.
PS_OUTPUT MainPS(PS_INPUT Input)
{
    float3 msd = msdTexture.Sample(msdSampler, Input.UV).rgb;
    
    float sd = median(msd.r, msd.g, msd.b) - 0.5f;
 
    float screenPxDistance = sd / fwidth(sd);
    float opacity = saturate(screenPxDistance + 0.5f);
    
    clip(opacity - 0.1f);
    
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);
    float3 finalColor = lerp(baseColor, ColorOverride, ColorOverrideAmount);
    
    // 색은 opacity로 블렌딩해 외곽을 부드럽게 하고,
    // 플래그/노멀 타깃은 알파 1로 덮어써서 DisableShading이 가장자리에서 섞이지 않게 한다.
    PS_OUTPUT Output;
    Output.GBufferA = float4(finalColor, Input.Color.a * opacity);
    Output.GBufferB = float4(0.5f, 0.5f, 1.0f, 1.0f);
    Output.GBufferC = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return Output;
    //return float4(1.0f, 0.0f, 0.0f, 1.0f); // 강제 빨간색 출력
}
    
   
