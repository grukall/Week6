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
    output.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    
    return output;
}