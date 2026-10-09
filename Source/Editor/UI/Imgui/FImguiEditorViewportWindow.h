#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Math/FVector2.h"
#include "ThirdParty/Imgui/imgui.h"
#include "FImguiStatsWindow.h"
#include "Runtime/Slate/SlateData.h"


// 3D 씬 위를 덮는 투명한 ImGui 창.
// - 다른 패널(ControlPanel, Property 등)이 이 창 위에 그려지므로,
//   ImGui 의 hover/active 판정이 "다른 패널에 가려지지 않은 뷰포트 영역"만 걸러준다.
// - 창의 위치와 크기를 활성 뷰포트의 UV로 되돌려주고 focus/hover 상태를 갱신한다.
// - 뷰포트 위에서 클릭이 발생하면 피킹을 수행한다.
class FImguiEditorViewportWindow final
{
	
	FImguiStatsWindow StatsWindow;
public:
	
	FImguiEditorViewportWindow() = default;
	~FImguiEditorViewportWindow() = default;

	//복사 생성 금지
	FImguiEditorViewportWindow(const FImguiEditorViewportWindow&) = delete;
	//복사 대입 금지
	FImguiEditorViewportWindow& operator=(const FImguiEditorViewportWindow&) = delete;


	void Process(FEditor& Editor, float DeltaTime);

	void Toggle(FImguiStatsWindow::EStatsWindow Window);
	void SetClose();

private:

	// ImGui 창을 열고 스타일을 적용한다. Process 가 EndWindow 로 짝을 맞춘다.
	void BeginWindow() const;
	void EndWindow() const;

	// ImGui 창의 실제 사각형을 뷰포트 UV 와 종횡비에 반영한다.
	// 사용자가 창을 옮기거나 크기를 바꾸면 3D 렌더 영역이 따라간다.
	void SyncViewportRect(FViewport& Viewport, FViewportClient& Client, const FRect& Rect, const FVector2& ClientSize) const;

	// 창 전체를 덮는 클릭 판정용 아이템을 만들고 입력 상태를 모은다.
	FViewportInput GatherInput(const FVector2& ViewportTopLeftPixels, const FVector2& ViewportSizePixels) const;

	// 창이 작업 영역 위로 올라가 타이틀바에 가리는 것을 막는다.
	void ClampWindowToWorkArea() const;

	void UpdateShortcuts(FEditor& Editor) const;

	void HandlePicking(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);
	void UpdateGizmoHover(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);
	void ShowViewportVerticalSplitter(SSplitter& Splitter);
	void ShowViewportHorizontalSplitter(SSplitter& Splitter);
	void ApplyPendingViewportMaximize(FEditor& Editor);
	bool GetViewportSceneRect(const ImVec2& Origin, FRect& OutRect) const;
	void DrawViewportHeader(int32 EntryIndex,FEditor& Editor);
	int32 PendingMaximizeViewport = -1;
};
