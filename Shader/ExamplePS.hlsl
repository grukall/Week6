#include "Constants.hlsli"

struct PS_INPUT
{
    float4 PosH : SV_Position;
    float3 PosW : Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 NormalW : NORMAL;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float3 BaseColor = lerp(Input.Color.rgb, ColorOverride, ColorOverrideAmount);

    if (DisableShading > 0.5f)
    {
        return float4(BaseColor, Input.Color.a);
    }
    
    float3 N = normalize(Input.NormalW);

    Material Mat;
    Mat.DiffAlbedo = BaseColor;
    Mat.SpecAlbedo = float3(1.0f, 1.0f, 1.0f);
    Mat.Shininess = 1.0f;
    
    float3 ToEye = normalize(CamPos - Input.PosW);
    
    float3 FinalColor = ComputeLight(DirLights, PointLights, SpotLights,
                    NumDirLights, NumPointLights, NumSpotLights,
                    Mat, Input.PosW, N, ToEye);
    
    float3 Ambient = Mat.DiffAlbedo * AmbientLight;
    return float4(Ambient + FinalColor, Input.Color.a);
    
}
