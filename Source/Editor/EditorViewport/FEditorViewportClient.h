#pragma once
#include "Runtime/Engine/FCamera.h"
#include "Editor/Grid/FGrid.h"
#include "Runtime/Engine/FViewportClient.h"
#include "Runtime/Engine/ShowFlags.h"
class UEngine;
class FViewport;

class FEditorViewportClient : public FViewportClient
{
	FGrid Grid;
	//Grid 이식중, ShowFlag 추가필요

	//연결된 Viewport
	FViewport* Viewport = nullptr;
public:
	FEditorViewportClient(UEngine* InEngine, uint32 InContextId) : FViewportClient(InEngine, InContextId)
	{
	}

	// Type에 따라 키보드,마우스 조작이 달라지기 때문에 ViewportClient에 있어야 한다고 생각함
	enum class EOrthogonalType {
		PERSPECTIVE,
		ORTHOGRAPHIC,
		ORTHOGRAPHIC_TOP,
		ORTHOGRAPHIC_BOTTOM,
		ORTHOGRAPHIC_LEFT,
		ORTHOGRAPHIC_RIGHT,
		ORTHOGRAPHIC_FRONT,
		ORTHOGRAPHIC_BACK,
	} eOrthogonalType = EOrthogonalType::PERSPECTIVE; 
	void SetOrthograpihcView(EOrthogonalType type);
	
	FCamera ViewportCamera;

	// 뷰포트 렌더 모드 및 쇼 플래그
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
					   static_cast<uint64>(EEngineShowFlags::SF_Grid);

	virtual bool IsOrtho() const override { return eOrthogonalType != EOrthogonalType::PERSPECTIVE; }
	virtual void AddAssociation(FViewport& _Viewport) override;
	virtual void RemoveAssociation(FViewport& _Viewport) override;

	[[nodiscard]] FViewport* GetViewport() const { return Viewport; }

	FGrid& GetGrid() { return Grid; }
	const FGrid& GetGrid() const { return Grid; }

	[[nodiscard]] bool HasShowFlag(EEngineShowFlags Flag) const {
		return (ShowFlags & static_cast<uint64>(Flag)) != 0;
	}
	
	void ToggleShowFlag(EEngineShowFlags Flag) {
		ShowFlags ^= static_cast<uint64>(Flag);
	}
};
