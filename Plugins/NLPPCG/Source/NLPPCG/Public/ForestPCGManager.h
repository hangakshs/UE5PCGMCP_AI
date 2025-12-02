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

	/** PCG Bounds를 정의하는 Box Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PCG")
	class UBoxComponent* BoundsComponent;

	/** PCG 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PCG")
	UPCGComponent* PCGComponent;

	/** MCP 클라이언트 참조 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MCP")
	AMCPClient* MCPClient;

	/**
	 * 나무 종류별 메시 매핑 (에디터에서 수정 가능)
	 * 키: 나무 타입 (pine, oak, birch, maple, generic_tree)
	 * 값: Static Mesh 에셋 경로
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest|Tree Meshes", meta = (DisplayName = "Tree Type Meshes"))
	TMap<FString, TSoftObjectPtr<UStaticMesh>> TreeMeshes;

	/** PCG 그래프 에셋 (자동 생성되거나 수동 할당) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PCG", meta = (DisplayName = "PCG Graph Asset"))
	UPCGGraph* PCGGraphAsset;

	/** PCG 그래프를 에셋으로 저장할지 여부 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PCG", meta = (DisplayName = "Save PCG Graph as Asset"))
	bool bSavePCGGraphAsAsset = true;

	/** PCG 그래프 에셋 저장 경로 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PCG", meta = (DisplayName = "Graph Asset Path", EditCondition = "bSavePCGGraphAsAsset"))
	FString GraphAssetPath = TEXT("/Game/PCG/Graphs/");

	/** 나무 메시 (하위 호환성을 위해 유지, TreeMeshes 사용 권장) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest|Legacy", meta = (DisplayName = "Default Tree Mesh (Legacy)"))
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

	/**
	 * 나무 타입에 맞는 메시 가져오기
	 * @param TreeType 나무 타입 (pine, oak, birch, maple, generic_tree)
	 * @return Static Mesh (없으면 기본 메시 반환)
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	UStaticMesh* GetTreeMeshForType(const FString& TreeType);

	/**
	 * PCG 그래프를 에셋으로 저장
	 * @return 저장 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "PCG", meta = (DisplayName = "Save PCG Graph as Asset"))
	bool SavePCGGraphAsAsset();

	/**
	 * 기본 나무 메시 맵 초기화 (에디터에서 호출 가능)
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest", meta = (DisplayName = "Initialize Default Tree Meshes"))
	void InitializeDefaultTreeMeshes();

	/**
	 * MCP 클라이언트 초기화 (에디터 모드에서 수동 호출 가능)
	 */
	UFUNCTION(BlueprintCallable, Category = "MCP")
	void InitializeMCPClient();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void OnForestParametersReceived(const FPCGForestParameters& Parameters);

	void SetupPCGGraph(const FPCGForestParameters& Parameters);
	void LoadDefaultTreeMesh();
};
