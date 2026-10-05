Texture2D SceneTexture : register(t6);
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
    
    float4 Sampled = SceneTexture.Sample(SceneSampler, Input.UV);
    
    output.Color = Sampled.rgba;
    
    return output;
}