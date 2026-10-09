#pragma once

#include "Runtime/Engine/UEngine.h"
#include "Editor/UI/Imgui/FImguiManager.h"
#include "Editor/UI/Imgui/FImguiToolBar.h"
#include "Editor/UI/Imgui/FImguiPropertyWindow.h"
#include "Editor/UI/Imgui/FImguiEditorViewportWindow.h"
#include "Editor/UI/Imgui/FImguiControlPanelWindow.h"
#include "Editor/UI/Imgui/FImguiConsoleWindow.h"
#include "Editor/UI/Imgui/FImguiWorldOutliner.h"
#include "Editor/UI/Imgui/FImguiContentsDrawer.h"
#include "Editor/Visualizer/FVisualizerRegistry.h"
#include "Editor/Application/FObjViewerApplication.h"
#include "Runtime/Core/TOptional.h"
#include "Editor/Core/FEditor.h"
#include "Runtime/Engine/UWorld.h"

class FViewport;

enum EPlaySessionType
{
	PIE,
	SIE
};

//세선 실행 요청
struct FRequestPlaySessionParams
{
	EWorldType WorldType;
	EPlaySessionType SessionType;
};

//PIE, SIE 실행 시 복구를 위한 상태 저장
struct FPlaySession
{
	//ContextId : 고유번호, UEngine::FindWorldContext로 찾을 수 있음
	uint32 PIEContextId;
	uint32 ViewEntryIndex;
	uint32 PrevViewContextId;
	EPlaySessionType SessionType;
};

class UEditorEngine : public UEngine
{
	GENERATED_BODY()
	DECLARE_UCLASS(UEditorEngine, UEngine)

protected:
	UEditorEngine(){}

public:
	virtual void Init(HWND Window) override;
	virtual void Exit() override;

	virtual void OnWindowResize(UINT Width, UINT Height) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Render() override;
	void ExecuteCommand(const char* Command);
	// 에디터 월드 (저장/로드/New의 대상). PIE 중에도 CurrentWorld와 무관하게 항상 Editor 컨텍스트의 월드를 가리킨다.
	[[nodiscard]] UWorld* GetEditorWorld() { return GetWorld(EditorContextId); }
	// PIE가 실행 중인지
	[[nodiscard]] bool IsPlaySessionActive() const { return PlaySession.IsSet(); }
	FEditor &GetEditor() { return Editor; }

	//PiE, SIE
	void StartQueuedPlaySessionRequest();
	void StartQueuedEndSessionRequest();
	void SwitchPlaySessionMode(FViewport* TargetViewport, EPlaySessionType SessionType);
	TOptional<FRequestPlaySessionParams> PlaySessionRequest;
	TOptional<FPlaySession> PlaySession;
	bool bRequestEndPlay = false;

private:
	void StartQueuedPlaySessionRequestInternal();

	//EWorldType::Editor 컨텍스트의 ID
	uint32 EditorContextId = InvalidContextId;


	FImguiEditorViewportWindow EditorViewportWindow;

#ifdef _OBJVIEWER
	//ObjViewer
	TUniquePtr<FObjViewerApplication> ObjViewer;
#else
	//Imgui
	FImguiManager ImguiManager;
	FImguiToolbar ToolBar;
	FImguiControlPanelWindow ControlPanelWindow;
	FImguiPropertyWindow PropertyWindow;
	FImguiConsoleWindow ConsoleWindow;
	FImguiWorldOutliner WorldOutliner;
	FImguiContentsDrawer ContentsDrawer;

	//Editor
	FEditor Editor;

	//Visualizer
	FVisualizerRegistry VisualizerRegistry;
#endif
};
