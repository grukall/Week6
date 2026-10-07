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
    
    // distance fog (밀도 * exp(-거리))
    float fn = Far - Near;
    float A = Far / fn;
    float B = -Far * Near / fn;
    float z_view = B / (Depth - A);
    
    // Height fog (밀도 * exp(-밀도감소율 * 높이))
    float z0 = 0.0f; // 기준 높이
    
    // 투영좌표 -> World 좌표 변환
    float ndcX = 2.0f * Input.UV.x - 1.0f;
    float ndcY = 1.0f - 2.0f * Input.UV.y;
    float4 clipPos = float4(ndcX, ndcY, Depth, 1.0f);
    float4 viewPos = mul(clipPos, InverseVP);

    float worldX = viewPos.x / viewPos.w;
    float worldY = viewPos.y / viewPos.w;
    float worldZ = viewPos.z / viewPos.w;
    float3 worldPos = float3(worldX, worldY, worldZ);
    
    float zp = worldZ;
    float L = length(worldPos - CamPos);
    float dz = (worldPos - CamPos) / L;

    A = Density * exp(-FogHeightFalloff * (z_view - CamPos));
    float heightFogFactor = A * (1 - exp(-FogHeightFalloff * dz * L)) / (FogHeightFalloff * dz);    // 안개 총량
    
    float fogDensity = 1 - exp(-heightFogFactor);   // 안개 비율
    
    output.Color = lerp(Sampled, FogColor, fogDensity);
    //output.Color = L / Far;
    return output;
}