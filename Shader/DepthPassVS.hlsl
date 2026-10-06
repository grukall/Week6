struct VS_INPUT
{
    uint InstanceID : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

VS_OUTPUT MainVS(VS_INPUT Input)
{
    VS_OUTPUT output;
    float2 dot0 = float2(0, 1);
    float2 dot1 = float2(2, 1);
    float2 dot2 = float2(0, -1);
    
    if (Input.InstanceID == 0)
    {
        output.Position = float4(-1.0, -1.0, 0.0, 1.0);
        output.UV = dot0;
    }
    else if (Input.InstanceID == 1)
    {
        output.Position = float4(3.0, -1.0, 0.0, 1.0);
        output.UV = dot1;
    }
    else if (Input.InstanceID == 2)
    {
        output.Position = float4(-1.0, 3.0, 0.0, 1.0);
        output.UV = dot2;
    }
    
    return output;
}