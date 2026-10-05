#pragma once

#include "FViewportClient.h"

class FGameViewportClient : public FViewportClient
{
public:
	FGameViewportClient(UEngine* InEngine) : FViewportClient(InEngine) {}

	virtual bool IsOrtho() const { return false; }
	virtual void AddAssociation(FViewport& Viewport) {}
	virtual void RemoveAssociation(FViewport& Viewport) {}
};