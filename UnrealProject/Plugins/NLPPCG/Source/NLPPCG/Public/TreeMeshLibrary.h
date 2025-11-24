// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TreeMeshLibrary.generated.h"

/**
 * 나무 종류별 메시 정보
 */
USTRUCT(BlueprintType)
struct FTreeMeshEntry
{
	GENERATED_BODY()

	/** 나무 타입 ID (pine, oak, birch, maple, generic_tree) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FString TreeType;

	/** 사용할 메시 배열 (계절별, 변종 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

	/** 기본 스케일 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FVector DefaultScale = FVector(1.0f, 1.0f, 1.0f);

	/** 스케일 변화 범위 (랜덤) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FVector ScaleVariation = FVector(0.2f, 0.2f, 0.2f);

	/** 무게 (선택 확률) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	float Weight = 1.0f;

	/** 계절 (Spring, Summer, Autumn, Winter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FString Season = TEXT("All");
};

/**
 * 나무 메시 라이브러리
 * 나무 종류별로 사용할 메시를 관리하는 데이터 에셋
 */
UCLASS(BlueprintType)
class NLPPCG_API UTreeMeshLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 등록된 나무 메시들 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Library")
	TArray<FTreeMeshEntry> TreeMeshes;

	/**
	 * 나무 타입으로 메시 찾기
	 *
	 * @param TreeType 나무 타입 (pine, oak, birch, maple, generic_tree)
	 * @param Season 계절 필터 (비어있으면 모든 계절)
	 * @return 해당하는 메시 엔트리들
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree Library")
	TArray<FTreeMeshEntry> GetMeshesByType(const FString& TreeType, const FString& Season = TEXT("")) const;

	/**
	 * 랜덤으로 메시 선택
	 *
	 * @param TreeType 나무 타입
	 * @param RandomStream 랜덤 스트림
	 * @return 선택된 메시 (없으면 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree Library")
	UStaticMesh* GetRandomMesh(const FString& TreeType, FRandomStream& RandomStream) const;

	/**
	 * 기본 스케일 및 변화 가져오기
	 */
	UFUNCTION(BlueprintCallable, Category = "Tree Library")
	void GetScaleInfo(const FString& TreeType, FVector& OutDefaultScale, FVector& OutScaleVariation) const;
};
