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
    
    if (MaxOpacity <= 0.0f)
    {
        output.Color = SceneTexture.Load(int3(Input.Position.xy, 0));
        return output;
    }
    
    const int2 pixelCoord = int2(Input.Position.xy);
    float4 Sampled = SceneTexture.Load(int3(pixelCoord, 0)); // 기존 색상 출력
    
    float Depth = DepthTexture.Load(int3(pixelCoord, 0)).r;
    float4 FogColor = float4(InscatteringColor.rgb, 1.0f);
    
    float z_view = 0.0f;
    {
        // distance fog (밀도 * exp(-거리))
        float fn = Far - Near;
        float A = Far / fn;
        float B = -Far * Near / fn;
        z_view = B / (Depth - A);
    }
        
    
    float fogDensity = 0.0f;
    {
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
        float L = max(length(worldPos - CamPos), 1e-4f);
        float dz = ((worldPos - CamPos) / L).z;

        // StartDistance 안쪽 구간에는 안개가 없다. 그만큼 광선의 시작점을 앞으로 옮긴다.
        float SkippedLength = min(StartDistance, L);
        float RayLength = L - SkippedLength;
        float RayStartZ = CamPos.z + dz * SkippedLength;

        // 높이에 따른 안개 농도 감소 (적분)
        float A = Density * exp(-FogHeightFalloff * (RayStartZ - z0));

        // 광선이 수평에 가까우면 분모가 0이 되므로 극한값(1)을 쓴다.
        float k = FogHeightFalloff * dz * RayLength;
        float Integral = abs(k) > 1e-4f ? (1 - exp(-k)) / k : 1.0f;
        float heightFogFactor = A * RayLength * Integral; // 안개 총량

        fogDensity = saturate(1 - exp(-heightFogFactor)); // 안개 비율

        // 아무리 짙어도 MaxOpacity 이상으로는 가리지 않는다.
        fogDensity = min(fogDensity, MaxOpacity);

        // CutoffDistance보다 먼 픽셀에는 안개를 적용하지 않는다. 0이면 제한 없음.
        if (CutoffDistance > 0.0f && L > CutoffDistance)
        {
            fogDensity = 0.0f;
        }
    }
    output.Color = lerp(Sampled, FogColor, fogDensity);
    
    //const float3 BackgroundColor = float3(0.5f, 0.5f, 0.5f);

    //// 아무 메시도 기록되지 않은 픽셀
    //if (Depth >= 0.999999f)
    //{
    //    output.Color = float4(BackgroundColor, 1.0f);
    //}

    return output;
    
}