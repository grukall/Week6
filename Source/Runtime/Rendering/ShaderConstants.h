#pragma once

#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"

#define MAXLIGHTS 16

// Register = b0
struct FFrameConstants {
	float Time;
	float DeltaTime;
	FVector2 Padding;
};
static_assert(sizeof(FFrameConstants) % 16 == 0);

// Register = b1
struct FViewConstants {
	FVector Pos;
	FMatrix View;
	FMatrix Projection;
	FMatrix InvVP;
	FVector2 ViewportSize;
	float Near;
	float Far;
};
static_assert(sizeof(FViewConstants) % 16 == 0);

// Register = b2
struct FObjectConstants {
  //FMatrix MVP;
  FVector4 Color{0.0f, 0.0f, 0.0f, 0.0f};
  FVector2 UVScale{1.0f, 1.0f};
  FVector2 UVOffset{0.0f, 0.0f};
  FMatrix World = FMatrix::GetIdentity();
  float DisableShading = 0.0f;
  FVector Padding;
};
static_assert(sizeof(FObjectConstants) % 16 == 0);

static constexpr uint32 ConstantRangeAlignment = 256u;

static constexpr uint32 AlignConstantRange(uint32 Size)
{
	return (Size + ConstantRangeAlignment - 1u)
		& ~(ConstantRangeAlignment - 1u);
}

static constexpr uint32 ObjectConstantStride = AlignConstantRange(sizeof(FObjectConstants));

static_assert(ObjectConstantStride % 256u == 0);
static_assert(sizeof(FObjectConstants) <= ObjectConstantStride);

static constexpr uint32 MaxObjectDrawCount = 16384u;

static constexpr uint32 ObjectConstantUploadBufferSize = ObjectConstantStride * MaxObjectDrawCount;

// Register = b3
//struct FShaderConstants {
//
//};
//static_assert(sizeof(FShaderConstants) % 16 == 0);


// Register = b2 / ObjectConstants Override
struct FGridConstants {
  FMatrix MVP;
  FMatrix World;
  float CellSize;
  FVector Padding;
};

static_assert(sizeof(FGridConstants) % 16 == 0);

// Register = b2 / ObjectConstants Override
// LINE_LIST 기반 에디터 그리드용 상수 버퍼
struct FGridLineConstants {
  FMatrix MVP;
  FVector CameraPosition;
  float FadeStartDistance;
  float FadeEndDistance;
  FVector Padding;
};

static_assert(sizeof(FGridLineConstants) % 16 == 0);

struct DirectionLight
{
	FVector Position{ 0.0f, 0.0f, 0.0f };
	float Intensity = 1.0f;	
	FVector LightColor{ 1.0f, 1.0f, 1.0f };
	float AmbientIntensity = 0.2f;

	FVector LightDirection{ -0.5f, -0.5f, -1.0f }; 
	float Padding = 0;
};

struct SpotLight
{
	FVector Position{ 0.0f, 0.0f, 0.0f };
	float Intensity = 1.0f;
	FVector LightColor{ 1.0f, 1.0f, 1.0f };
	float AmbientIntensity = 0.2f;

	FVector LightDirection{ -0.5f, -0.5f, -1.0f }; 
	float SpotPower = 64.0f;
	
	float FallOffStart = 0.0f;	// Point, Spot Light Only
	float FallOffEnd = 0.0f;	// Point, Spot Light Only
	float Padding[2]{};
};

struct PointLight
{
	FVector Position{ 0.0f, 0.0f, 0.0f };
	float Intensity = 1.0f;
	FVector LightColor{ 1.0f, 1.0f, 1.0f };
	float AmbientIntensity = 0.2f;

	float FallOffStart = 0.0f;	// Point, Spot Light Only
	float FallOffEnd = 0.0f;	// Point, Spot Light Only
	float PaddingB[2]{};
};


// 나중에 수정 필요
// Register = b4
struct FLightConstants {
	
	int32 NumDirLights = 0;
	int32 NumSpotLights = 0;
	int32 NumPointLights = 0;

	int32 Padding = 0;

	FVector AmbientLight{0.25f, 0.25f, 0.35f}; float Padding1 = 0;

	DirectionLight DirLights[MAXLIGHTS];
	SpotLight SpotLights[MAXLIGHTS];
	PointLight PointLights[MAXLIGHTS];
};

static_assert(sizeof(FLightConstants) % 16 == 0);


// 머티리얼
// Register = b5
struct FMaterialConstants
{
	FVector DiffAlbedo{};
	float Shininess = 0.0f;
	FVector SpecAlbedo{};
	float Padding = 0.0f;
};

static_assert(sizeof(FMaterialConstants) % 16 == 0);


// 안개 데이터
// Register = b6
struct FFogData
{
	FMatrix InverseVP;
	float Density = 0.5f;
	float HeightFalloff = 0.1f;
	float StartDistance = 10.0f;
	float CutoffDistance = 1000.0f;
	float MaxOpacity = 0.8f;
	FVector4 InscatteringColor{ 0.0f, 1.0f, 0.0f, 1.0f };
};

static_assert(sizeof(FFogData) % 16 == 0);