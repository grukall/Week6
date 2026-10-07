#define Center 0
#define NW 1
#define NE 2
#define SW 3
#define SE 4

#define THICKNESS 1
#define FXAAReduceMul (1.0f / 8.0f)
#define FXAAReduceMin (1.0f / 128.0f)
#define FXAASpanMax 8.0f
#define SUBPIX

// 외곽선 후처리 픽셀 셰이더
Texture2D<float4> SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PS_IN
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float RGB2luma(float4 Color)
{
    return dot(Color, float4(0.299, 0.587, 0.114, 0.0));

}

float3 Coord2Color(float2 UV)//int2 Coord)
{
    return SceneTexture.SampleLevel(LinearSampler, UV, 0).rgb;
    //return SceneTexture.Load(int3(Coord, 0));
}

float TextureOffset(float2 UV)//int2 Coord)
{
    return RGB2luma(SceneTexture.SampleLevel(LinearSampler, UV, 0));
    //return RGB2luma(SceneTexture.Load(int3(Coord, 0)));
}

float Average5(float arr[5])
{
    float total = 0;
    for (int i = 0; i < 5; ++i)
        total += arr[i];
    return total / 5.0f;
}


float4 MainPS(PS_IN input) : SV_TARGET
{
    const int2 pixelCoord = int2(input.Pos.xy);
    uint width, height;
    SceneTexture.GetDimensions(width, height);
    float2 InvResolution = float2(1.0 / float2(width, height));
    
    float2 UV = input.Pos.xy * InvResolution;
    float4 PixelColor = SceneTexture.SampleLevel(LinearSampler, UV, 0);
    // const float4 PixelColor = SceneTexture.Load(int3(pixelCoord, 0));
    
    //float luma[5] = {0,0,0,0,0};
    //luma[Center] = RGB2luma(PixelColor);
    //luma[NW] = TextureOffset(int2(-THICKNESS, -THICKNESS));
    //luma[NE] = TextureOffset(int2(-THICKNESS, THICKNESS));
    //luma[SW] = TextureOffset(int2(THICKNESS, -THICKNESS));
    //luma[SE] = TextureOffset(int2(THICKNESS, THICKNESS));
    
    float luma[5];
    luma[Center] = RGB2luma(PixelColor);
    luma[NW] = TextureOffset(UV + float2(-1, -1) * InvResolution);
    luma[NE] = TextureOffset(UV + float2(1, -1) * InvResolution);
    luma[SW] = TextureOffset(UV + float2(-1, 1) * InvResolution);
    luma[SE] = TextureOffset(UV + float2(1, 1) * InvResolution);
    
    float lumaMin = min(luma[Center], min(min(luma[NW], luma[NE]), min(luma[SW], luma[SE])));
    float lumaMax = max(luma[Center], max(max(luma[NW], luma[NE]), max(luma[SW], luma[SE])));
    float lumaRange = lumaMax - lumaMin;
    float threashold = max(0.0625, lumaMax * 0.125);
        
    if (lumaRange < threashold)
    {
        return PixelColor;
    }

    float2 dir;
    dir.x = -((luma[NW] + luma[NE]) - (luma[SW] + luma[SE]));
    dir.y = (luma[NW] + luma[SW]) - (luma[NE] + luma[SE]);
   
    float dirReduce = max((Average5(luma) * FXAAReduceMul), FXAAReduceMin);
    float rcpDirMin = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    float2 dirScaled = dir * rcpDirMin;
   
    dirScaled = clamp(dirScaled,
    float2(-FXAASpanMax, -FXAASpanMax), float2(FXAASpanMax, FXAASpanMax));

    //dir = dirScaled * InvResolution;
    
    dir = dirScaled * InvResolution;

    
    //float3 rgbA = 0.5f * (
    //    Coord2Color(pixelCoord + dir * (1.0 / 3.0 - 0.5)) +
    //    Coord2Color(pixelCoord + dir * (2.0 / 3.0 - 0.5))
    //);
    
    //float3 rgbB = rgbA * 0.5f + 0.25f * (
    //    Coord2Color(pixelCoord + dir * - 0.5) +
    //    Coord2Color(pixelCoord + dir * 0.5)
    //);
    
    float3 rgbA = 0.5f * (
    Coord2Color(UV + dir * (1.0f / 3.0f - 0.5f)) +
    Coord2Color(UV + dir * (2.0f / 3.0f - 0.5f))
);

    float3 rgbB = rgbA * 0.5f + 0.25f * (
    Coord2Color(UV - dir * 0.5f) +
    Coord2Color(UV + dir * 0.5f)
);
    
    float lumaB = RGB2luma(float4(rgbB, 0));
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        rgbB = rgbA;
    }
    
    const float Subpix = 0.5f;
    float4 FinalColor = lerp(PixelColor, float4(rgbB, 0), Subpix);
    
    return float4(rgbB, PixelColor.a);
    //return FinalColor;
}