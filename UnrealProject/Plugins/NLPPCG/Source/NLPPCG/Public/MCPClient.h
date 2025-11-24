// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "PCGForestGenerator.h"
#include "MCPClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMCPResponse, const FString&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnForestGenerated, const FPCGForestParameters&, Parameters);

/**
 * MCP 서버와 통신하는 클라이언트 Actor
 * Python MCP 서버에 자연어 명령을 전송하고 PCG 파라미터를 수신합니다.
 */
UCLASS(Blueprintable, BlueprintType)
class NLPPCG_API AMCPClient : public AActor
{
	GENERATED_BODY()

public:
	AMCPClient();

	/** MCP 서버 URL (예: http://localhost:8000) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	FString ServerURL = TEXT("http://localhost:8000");

	/** 디버그 모드 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	bool bDebugMode = true;

	/** 파일 기반 통신 사용 (MCP 서버가 stdio 모드일 때) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	bool bUseFileCommunication = true;

	/** 파일 폴링 간격 (초) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	float FilePollingInterval = 0.5f;

	/** MCP 응답 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "MCP")
	FOnMCPResponse OnMCPResponse;

	/** 숲 생성 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "MCP")
	FOnForestGenerated OnForestGenerated;

	/**
	 * 자연어 명령을 MCP 서버로 전송
	 * @param Command 자연어 명령 (예: "밀집된 소나무 숲 만들어줘")
	 */
	UFUNCTION(BlueprintCallable, Category = "MCP")
	void SendCommand(const FString& Command);

	/**
	 * 숲 제거 명령 전송
	 */
	UFUNCTION(BlueprintCallable, Category = "MCP")
	void ClearForest();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void HandleHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void ProcessForestCommand(const FString& JsonResponse);
	FPCGForestParameters ParseForestParameters(TSharedPtr<FJsonObject> ParamsObject);

	// 파일 기반 통신
	void CheckCommandFile();
	FString GetProjectIntermediatePath() const;
	void SendCommandViaFile(const FString& Command);

	// 타이머
	float TimeSinceLastPoll = 0.0f;
	FString LastProcessedCommandHash;
};
