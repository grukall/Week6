#include "LightingUtil.hlsli"

cbuffer FrameConstants : register(b0)
{
    float Time;
    float DeltaTime;
    float2 FramePadding;
}

cbuffer ViewConstants : register(b1)
{
    float3 CamPos;
    row_major float4x4 View;
    row_major float4x4 Projection;
    float2 ViewportSize;
    float2 ViewPadding;
}

cbuffer ObjectConstants : register(b2)
{
    //row_major float4x4 MVP;
    float3 ColorOverride;
    float ColorOverrideAmount;
    float2 UVScale;
    float2 UVOffset;
    row_major float4x4 World;
    float DisableShading;
    float3 ObjectPadding;
}

cbuffer LightConstants : register(b4)
{
    int NumDirLights;
    int NumSpotLights;
    int NumPointLights;

    float3 AmbientLight;
    
    DirectionLight DirLights[MAXLIGHTS];
    SpotLight SpotLights[MAXLIGHTS];
    PointLight PointLights[MAXLIGHTS];

    float Padding = 0.0f;
};

/*
float3 LightDirection;
    float Intensity;
    float3 LightColor;
    float AmbientIntensity;
    float SpotIntensity; 
    float FallOffStart;
    float FallOffEnd;
    float3 Position;*/

cbuffer MaterialConstants : register(b5)
{
    float3 diffAlbedo;
    float Shininess;
    float3 specAlbedo;
}
