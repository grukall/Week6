#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Asset/UTexture.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Rendering/FTexture.h"
#include "Runtime/Material/FTextureSamplerDesc.h"

struct UMaterialDesc : UAssetDesc
{
	UPipeline* Pipeline;
	UTexture* Texture;
	FTextureSamplerDesc TextureSamplerDesc;
	FVector Diffuse{ 1.0f, 1.0f, 1.0f };
	float Shininess = 0.0f;
	FVector Specular{ 0.0f, 0.0f, 0.0f };
};

class UMaterial : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UMaterial, UAsset)

private:

	UPipeline* Pipeline = nullptr;
	UTexture* Texture = nullptr;
	FTextureSamplerDesc SamplerDesc{};

	FVector Diffuse{};
	float Shininess = 0.0f;
	FVector Specular{};

public:

	void Load(UMaterialDesc& Desc);

	UPipeline* GetPipeline() const { return Pipeline; }
	UTexture* GetTexture() const { return Texture; }
	FTextureSamplerDesc GetSamplerDesc() const { return SamplerDesc; }

	FVector GetDiffuse() { return Diffuse; }
	FVector GetSpecular() { return Specular; }
	float GetShininess() { return Shininess; }

};