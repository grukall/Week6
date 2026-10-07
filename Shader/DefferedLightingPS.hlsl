#include "Constants.hlsli"

Texture2D<float4> GBufferA : register(t6);
Texture2D<float4> GBufferB : register(t7);
Texture2D<float4> GBufferC : register(t8);
Texture2D<float> SceneDepth : register(t9);

SamplerState GBufferSampler : register(s0);

struct PS_INPUT
{
    float4 PosH : SV_Position;
    float2 UV : TEXCOORD0;
};

float3 ReconstructWorldPosition(PS_INPUT Input)
{
    // SV_Position은 전체 렌더 타깃의 픽셀 좌표
    float Depth = SceneDepth.Load(int3(int2(Input.PosH.xy), 0)).r;

    // UV는 현재 뷰포트 내부의 0~1 좌표
    float2 NDC = float2(
        Input.UV.x * 2.0f - 1.0f,
        1.0f - Input.UV.y * 2.0f
    );

    // D3D의 깊이 범위는 0~1이므로 Depth는 그대로 사용
    float4 ClipPosition = float4(NDC, Depth, 1.0f);
    float4 WorldPosition = mul(ClipPosition, InvVP);

    return WorldPosition.xyz / WorldPosition.w;
}

float4 MainPS(PS_INPUT Input) : SV_Target0
{
    int3 Pixel = int3(int2(Input.PosH.xy), 0);
    float4 A = GBufferA.Load(Pixel);
    float4 B = GBufferB.Load(Pixel);
    float4 C = GBufferC.Load(Pixel);
    float Depth = SceneDepth.Load(Pixel).r;

    // A.rgb: DiffAlbedo = diffAlbedo * BaseColor, A.a: Shininess
    // B.rgb: 인코딩된 NormalW, B.a: DisableShading
    // C.rgb: specAlbedo
    float3 NormalW = normalize(B.rgb * 2.0f - 1.0f);

    // Input.UV와 Depth로 월드 위치 복원
    float3 PosW = ReconstructWorldPosition(Input);
    
    // 복원한 위치와 현재 광원으로 조명 계산
    
    float3 ToEye = normalize(CamPos - PosW);
    
    Material Mat;
    Mat.DiffAlbedo = A.rgb;
    Mat.SpecAlbedo = C.rgb;
    Mat.Shininess = A.a;
    
    float3 FinalColor = ComputeLight(DirLights, PointLights, SpotLights,
                        NumDirLights, NumPointLights, NumSpotLights,
                        Mat, PosW, NormalW, ToEye);
    
    float3 Ambient = Mat.DiffAlbedo * AmbientLight;
    
    // return float4(이번 광원의 조명값, 1.0f);
    return float4(Ambient + FinalColor, 1.0f);
    
    //return float4(Depth.xxx, 1.0f); // 우선 GBuffer 확인용

    //return float4(1.0f, 0.0f, 0.0f, 1.0f);

}