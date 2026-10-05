#pragma once

#include "FViewportClient.h"

class FGameViewportClient : public FViewportClient
{
public:
	FGameViewportClient(UEngine* InEngine, uint32 InContextId) : Engine(InEngine), ContextId(InContextId) {}

	virtual bool IsOrtho() const { return false; }

	//Viewport 등록 이외에 클라이언트별 추가 작업이 필요한 경우, 여기에 구현한다.
	virtual void AddAssociation(FViewport& Viewport) {}
	virtual void RemoveAssociation(FViewport& Viewport) {}
};