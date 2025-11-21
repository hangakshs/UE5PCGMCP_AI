// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ForestPCGManagerLibrary.generated.h"

class AForestPCGManager;

/**
 * ForestPCGManager 자동 설정을 위한 블루프린트 함수 라이브러리
 */
UCLASS()
class NLPPCG_API UForestPCGManagerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 레벨에서 ForestPCGManager를 찾거나 자동으로 생성
	 * @param WorldContext World Context
	 * @param SpawnLocation ForestPCGManager 생성 위치 (없으면 자동 생성 시 사용)
	 * @return ForestPCGManager 인스턴스
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static AForestPCGManager* GetOrCreateForestPCGManager(
		UObject* WorldContextObject,
		FVector SpawnLocation = FVector::ZeroVector
	);

	/**
	 * 자연어 명령으로 숲 생성 (ForestPCGManager 자동 생성 포함)
	 * @param WorldContext World Context
	 * @param Command 자연어 명령 (예: "밀집된 소나무 숲")
	 * @param SpawnLocation Manager 생성 위치
	 * @return 생성 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static bool GenerateForestFromNLP(
		UObject* WorldContextObject,
		const FString& Command,
		FVector SpawnLocation = FVector::ZeroVector
	);

	/**
	 * 모든 ForestPCGManager의 PCG 그래프를 에셋으로 저장
	 * @param WorldContext World Context
	 * @return 저장된 Manager 수
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static int32 SaveAllPCGGraphsAsAssets(UObject* WorldContextObject);

	/**
	 * ForestPCGManager 자동 설정 (나무 메시 초기화 + PCG 그래프 생성)
	 * @param Manager 설정할 ForestPCGManager
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG")
	static void AutoSetupForestPCGManager(AForestPCGManager* Manager);
};
