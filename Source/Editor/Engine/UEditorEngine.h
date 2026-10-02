#pragma once

#include "Editor/Application/IApplication.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Engine/UEngine.h"

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

private:
	TUniquePtr<IApplication> Application;
};
