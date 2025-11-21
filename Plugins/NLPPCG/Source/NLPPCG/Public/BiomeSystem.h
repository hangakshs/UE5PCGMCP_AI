// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PCGForestGenerator.h"
#include "BiomeSystem.generated.h"

/**
 * 바이옴 타입
 */
UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	ConiferousForest UMETA(DisplayName = "침엽수림 (Coniferous Forest)"),
	DeciduousForest UMETA(DisplayName = "활엽수림 (Deciduous Forest)"),
	MixedForest UMETA(DisplayName = "혼합림 (Mixed Forest)"),
	TropicalRainforest UMETA(DisplayName = "열대우림 (Tropical Rainforest)"),
	Taiga UMETA(DisplayName = "타이가 (Taiga)"),
	TemperateRainforest UMETA(DisplayName = "온대우림 (Temperate Rainforest)")
};

/**
 * 나무 종 분포 정보
 */
USTRUCT(BlueprintType)
struct FTreeSpeciesDistribution
{
	GENERATED_BODY()

	/** 나무 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Species")
	FString TreeType;

	/** 이 바이옴에서의 비율 (0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Species", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Proportion = 0.25f;

	/** 우세종 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Species")
	bool bDominantSpecies = false;
};

/**
 * 바이옴 설정
 */
USTRUCT(BlueprintType)
struct FBiomeSettings
{
	GENERATED_BODY()

	/** 바이옴 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	EBiomeType BiomeType = EBiomeType::MixedForest;

	/** 바이옴 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FString BiomeName = TEXT("Mixed Forest");

	/** 나무 종 분포 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	TArray<FTreeSpeciesDistribution> SpeciesDistribution;

	/** 기본 PCG 파라미터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FPCGForestParameters BaseParameters;

	/** 층위 구조 활성화 (교목, 아교목, 관목) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	bool bEnableStratification = false;

	/** 교목 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Stratification", meta = (EditCondition = "bEnableStratification", ClampMin = "0.0", ClampMax = "1.0"))
	float CanopyProportion = 0.6f;

	/** 아교목 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Stratification", meta = (EditCondition = "bEnableStratification", ClampMin = "0.0", ClampMax = "1.0"))
	float UnderstoryProportion = 0.3f;

	/** 관목 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Stratification", meta = (EditCondition = "bEnableStratification", ClampMin = "0.0", ClampMax = "1.0"))
	float ShrubProportion = 0.1f;

	/** 기후 조건: 최소 온도 (섭씨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate")
	float MinTemperature = -10.0f;

	/** 기후 조건: 최대 온도 (섭씨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate")
	float MaxTemperature = 30.0f;

	/** 기후 조건: 연평균 강수량 (mm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate")
	float AnnualPrecipitation = 1000.0f;
};

/**
 * 바이옴 데이터 에셋
 * 다양한 바이옴 프리셋을 관리
 */
UCLASS(BlueprintType)
class NLPPCG_API UBiomeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UBiomeDataAsset();

	/** 등록된 바이옴들 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes")
	TArray<FBiomeSettings> Biomes;

	/**
	 * 바이옴 타입으로 설정 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FBiomeSettings GetBiomeSettings(EBiomeType BiomeType) const;

	/**
	 * 이름으로 바이옴 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FBiomeSettings GetBiomeByName(const FString& BiomeName) const;

	/**
	 * 바이옴에서 랜덤 나무 종 선택
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FString SelectRandomTreeSpecies(const FBiomeSettings& Biome, FRandomStream& RandomStream) const;

	/**
	 * 기본 바이옴 프리셋 생성
	 */
	void CreateDefaultBiomes();
};

/**
 * 바이옴 기반 숲 생성기
 */
UCLASS(BlueprintType)
class NLPPCG_API UBiomeForestGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 바이옴에 맞는 PCG 파라미터 생성
	 *
	 * @param BiomeSettings 바이옴 설정
	 * @param OutParameters 출력 파라미터 배열 (층위별)
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome Generator")
	static bool GenerateParametersFromBiome(
		const FBiomeSettings& BiomeSettings,
		TArray<FPCGForestParameters>& OutParameters
	);

	/**
	 * 기후 조건으로 적합한 바이옴 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome Generator")
	static EBiomeType FindSuitableBiome(
		float Temperature,
		float Precipitation,
		const UBiomeDataAsset* BiomeData
	);
};
