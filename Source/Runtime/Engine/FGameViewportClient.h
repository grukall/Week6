#pragma once

#include "FViewportClient.h"
#include "Runtime/Slate/SlateData.h"

class FGameViewportClient : public FViewportClient
{
	//Game 클라이언트에서 가져올 카메라가 없을 때, Editor 클라이언트처럼 이 카메라를 사용한다.
	FCamera TempCamera;
public:
	FGameViewportClient(UEngine* InEngine, uint32 InContextId, const FCamera &EditorCamera)
		: FViewportClient(InEngine, InContextId), TempCamera(EditorCamera)
	{}

	virtual bool IsOrtho() const { return false; }

	//Viewport 등록 이외에 클라이언트별 추가 작업이 필요한 경우, 여기에 구현한다.
	virtual void AddAssociation(FViewport& Viewport) override;
	virtual void RemoveAssociation(FViewport& Viewport) override;
	virtual FCamera* GetCamera() { return &TempCamera; }
	virtual void ProccessInput(const FViewportInput& Input, float deltaTime);
};