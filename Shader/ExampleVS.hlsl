#include "Constants.hlsli"

//struct VS_INPUT
//{
//    float3 Position : POSITION;
//    float4 Color : COLOR;
//    float2 UV : TEXCOORD0;
//    float3 Normal : NORMAL;
//};

//struct PS_INPUT
//{
//    float4 Position : SV_Position;
//    float4 Color : COLOR;
//    float2 UV : TEXCOORD0;
//    float3 Normal : NORMAL;
//};

struct VS_INPUT
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 PosH : SV_Position;
    float3 PosW : Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 NormalW : NORMAL;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    Output.PosW = mul(float4(Input.PosL, 1.0f), World);
    Output.PosH = mul(float4(Input.PosL, 1.0f), mul(World, mul(View, Projection)));
    Output.Color = Input.Color;
    Output.UV = Input.UV * UVScale + UVOffset;

    // 월드 공간 법선 변환
    Output.NormalW = normalize(mul(float4(Input.Normal, 0.0f), World).xyz);

    return Output;
}
