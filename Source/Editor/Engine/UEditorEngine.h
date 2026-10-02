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
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/PointerTypes.h"

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
	virtual void Update(float DeltaTime) override;
	virtual void Render() override;
	void ExecuteCommand(const char* Command);

private:

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
