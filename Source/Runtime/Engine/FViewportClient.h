#pragma once
#include "Runtime/Core/intTypes.h"

// FViewport.h와 서로 include하면 순환되므로 전방 선언만 사용한다.
class FViewport;
class UWorld;
class UEngine;

//뷰포트에 어떤 월드 정보를 그릴지 구현하는 계층
//GetWorld : 이 클라이언트가 표현하는 UWorld 포인터
//IsOrtho : 클라이언트 카메라 직교 투영 여부
//AddAssociation, RemoveAssociation : 클라이언트가 표현하는 뷰포트 등록/제거, 뷰포트에 표현하는 World를 교체해야 할 때 사용
class FViewportClient
{

public:

	FViewportClient(UEngine* InEngine, uint32 InContextId) : Engine(InEngine), ContextId(InContextId) {}

	virtual ~FViewportClient() = default;
	UWorld* GetWorld();
	virtual bool IsOrtho() const { return false;}
	virtual void AddAssociation(FViewport& Viewport) {}
	virtual void RemoveAssociation(FViewport& Viewport) {}

protected:
	UEngine* Engine = nullptr;

	//WorldContext 고유번호 저장, GetWorld에서 사용
	uint32 ContextId = -1;
};
