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
    
    // float4 Sampled = SceneTexture.Sample(SceneSampler, Input.UV); // 기존 색상 출력
    float4 Depth = DepthTexture.Sample(SceneSampler, Input.UV).r;
    
    output.Color = Depth.rgba;
    
    return output;
}