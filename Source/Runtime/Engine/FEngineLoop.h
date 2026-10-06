#pragma once

#include "Runtime/Core/PointerTypes.h"
#include "Runtime/ApplicationCore/FWindowApplication.h"

class UEngine;
class FTimeManager;

// 엔진의 하부 계층을 담당하는 클래스
// 프로세스단의 처리가 필요한 부분은 여기서 담당
class FEngineLoop
{
private:
	TUniquePtr<FWindowsApplication> WindowsApplication;
	UEngine* Engine = nullptr;

public:

	void Init(HINSTANCE Instance);
	void SetEngine(UEngine* InEngine) { Engine = InEngine; }

	void Tick();
	void Exit();

	HWND GetMainWindowHandle() const;

private:

	bool CheckWindowMessage();

};
