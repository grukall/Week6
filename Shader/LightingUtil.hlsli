#define MAXLIGHTS 16

// 44 Bytes
struct DirectionLight
{
    float Intensity;
    float AmbientIntensity;
    float3 LightColor;
    float3 Position;
    float3 LightDirection; // Direction, Spot Light Only
};

// 40 Bytes
struct SpotLight
{
    float Intensity;
    float AmbientIntensity;
    float3 LightColor;
    float3 Position;
    float3 LightDirection; // Direction, Spot Light Only
    float FallOffStart; // Point, Spot Light Only
    float FallOffEnd; // Point, Spot Light Only
    float SpotPower;
};

// 40 Bytes
struct PointLight
{
    float Intensity;
    float AmbientIntensity;
    float3 LightColor;
    float3 Position;
    float FallOffStart; // Point, Spot Light Only
    float FallOffEnd; // Point, Spot Light Only
};

struct Material
{
    float3 DiffAlbedo;  // 
    float3 SpecAlbedo;
    float Shininess;
};

float CalcAttenuation(float D, float FallOffStart, float FallOffEnd )
{
    return saturate((FallOffEnd - D) / (FallOffEnd - FallOffStart));
}

/***
Schlick's Approximation : Fresnel reflectance 근사 
표면에서 얼마나 많은 빛이 반사되는가
표면 법선과 빛/시야 방향 사이의 각도를 나타내는 함수
R0 : 정반사에서의 반사율
***/
float3 SchlickFresnel(float3 R0, float3 Normal, float3 LightVec)
{
    float CosIncidentAngle = saturate(dot(Normal, LightVec));
    float f0 = 1.0f - CosIncidentAngle;
    float3 ReflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    return ReflectPercent;
}

/***
LightStrength : Intensity * LightColor
***/
float3 BlinnPhong(float3 LightStrength, float3 LightVec,
                   float3 Normal, float3 ToEye, Material Mat)
{
    float3 HalfVec = normalize(ToEye + LightVec);
    const float M = Mat.Shininess * 256.0f;
    float RoughnessFactor = (M + 8.0f) * pow(max(dot(HalfVec, Normal), 0.0f), M) / 8.0f;
    
    float3 SpecFactor = Mat.SpecAlbedo * RoughnessFactor;
    SpecFactor = SpecFactor / (SpecFactor + 1.0f);
    
    return (Mat.DiffAlbedo + SpecFactor) * LightStrength;
}


float3 ComputeDirectionalLight(DirectionLight L, Material Mat, float3 Normal, float3 ToEye)
{
    float3 LightVec = -L.LightDirection;
    
    // Lambert's Cosine Law
    float NdotL = max(dot(LightVec, Normal), 0.0f);
    float3 LightStrength = L.Intensity * L.LightColor * NdotL;
    return BlinnPhong(LightStrength, LightVec, Normal, ToEye, Mat);
}

float3 ComputePointLight(PointLight L, Material Mat, float3 Pos, float3 Normal, float3 ToEye)
{
    float3 LightVec = L.Position - Pos;
    float D = length(LightVec);
    if (D > L.FallOffEnd)
    {
        return 0.0f;
    }
    
    LightVec /= D;
    
    float NdotL = max(dot(LightVec, Normal), 0.0f);
    float3 LightStrength = L.Intensity * L.LightColor * NdotL;
    LightStrength *= CalcAttenuation(D, L.FallOffStart, L.FallOffEnd);
    
    return BlinnPhong(LightStrength, LightVec, Normal, ToEye, Mat);
}

float3 ComputeSpotLight(SpotLight L, Material Mat, float3 Pos, float3 Normal, float3 ToEye)
{
    float3 LightVec = L.Position - Pos;
    float D = length(LightVec);
    if (D > L.FallOffEnd)
    {
        return 0.0f;
    }
    
    LightVec /= D;
    
    float NdotL = max(dot(LightVec, Normal), 0.0f);
    float3 LightStrength = L.Intensity * L.LightColor * NdotL;
    
    float Att = CalcAttenuation(D, L.FallOffStart, L.FallOffEnd);
    
    float SpotFactor = pow(max(1 - LightVec, L.LightDirection), L.SpotPower);
    LightStrength *= Att * SpotFactor;
    
    return BlinnPhong(LightStrength, LightVec, Normal, ToEye, Mat);
}

float3 ComputeLight(DirectionLight DirLights[MAXLIGHTS], 
                    PointLight PointLights[MAXLIGHTS], 
                    SpotLight SpotLights[MAXLIGHTS],
                    int NumDirLights, int NumPointLights, int NumSpotLights,
                    Material Mat, float3 Pos, float3 Normal, float3 ToEye)
{
    float3 Result = { 0.0f, 0.0f, 0.0f };
    
    for (int i = 0; i < NumDirLights; ++i)
    {
        Result += ComputeDirectionalLight(DirLights[i], Mat, Normal, ToEye);
    }
    
    for (int j = 0; j < NumPointLights; ++j)
    {
        Result += ComputePointLight(PointLights[i], Mat, Pos, Normal, ToEye);
    }
    
    for (int k = 0; k < NumSpotLights; ++k)
    {
        Result += ComputeSpotLight(SpotLights[i], Mat, Pos, Normal, ToEye);
    }
    
    return Result;
}