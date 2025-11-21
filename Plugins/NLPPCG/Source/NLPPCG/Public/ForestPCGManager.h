// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "PCGComponent.h"
#include "PCGForestGenerator.h"
#include "MCPClient.h"
#include "ForestPCGManager.generated.h"

/**
 * PCG 기반 숲 관리 Actor
 * MCP 클라이언트와 연동하여 자연어 명령으로 숲을 생성/제거합니다.
 */
UCLASS(Blueprintable, BlueprintType)
class NLPPCG_API AForestPCGManager : public AActor
{
	GENERATED_BODY()

public:
	AForestPCGManager();

	/** PCG 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PCG")
	UPCGComponent* PCGComponent;

	/** MCP 클라이언트 참조 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	AMCPClient* MCPClient;

	/** 나무 메시 (임시로 큐브 메시 사용) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	UStaticMesh* TreeMesh;

	/** 자동으로 MCP 클라이언트 생성 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	bool bAutoCreateMCPClient = true;

	/**
	 * 자연어 명령으로 숲 생성
	 * @param Command 자연어 명령
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	void GenerateForestFromNLP(const FString& Command);

	/**
	 * 숲 제거
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	void ClearForest();

	/**
	 * 파라미터로 직접 숲 생성
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	void GenerateForestFromParameters(const FPCGForestParameters& Parameters);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UFUNCTION()
	void OnForestParametersReceived(const FPCGForestParameters& Parameters);

	void SetupPCGGraph(const FPCGForestParameters& Parameters);
	void LoadDefaultTreeMesh();
};
