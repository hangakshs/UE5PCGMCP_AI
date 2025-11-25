// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGTerrainAdapter.generated.h"

/**
 * 지형 필터 설정
 */
USTRUCT(BlueprintType)
struct FTerrainFilterSettings
{
	GENERATED_BODY()

	/** 최소 경사 (도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MinSlope = 0.0f;

	/** 최대 경사 (도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxSlope = 45.0f;

	/** 최소 고도 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float MinAltitude = -100000.0f;

	/** 최대 고도 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float MaxAltitude = 100000.0f;

	/** 경사에 따른 밀도 조정 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAdjustDensityBySlope = true;

	/** 경사에 따른 스케일 조정 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAdjustScaleBySlope = true;

	/** 지형에 정렬 (나무가 경사면을 따라 회전) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAlignToTerrain = true;

	/** 정렬 강도 (0=정렬 안함, 1=완전 정렬) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlignmentStrength = 0.7f;
};

/**
 * PCG 지형 어댑터 노드
 * 포인트를 지형에 맞게 필터링하고 조정합니다
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class NLPPCG_API UPCGTerrainAdapterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGTerrainAdapterSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("TerrainAdapter")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGTerrainAdapter", "NodeTitle", "Terrain Adapter"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Filter; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	/** 지형 필터 설정 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FTerrainFilterSettings TerrainFilter;

	/** 레이캐스트 거리 (위에서 아래로) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	float RaycastDistance = 10000.0f;

	/** 디버그 표시 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bShowDebug = false;
};

/**
 * PCG 지형 어댑터 엘리먼트
 */
class FPCGTerrainAdapterElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;

private:
	/**
	 * 지형 법선 계산
	 */
	FVector CalculateTerrainNormal(UWorld* World, const FVector& Location, float SampleDistance = 100.0f) const;

	/**
	 * 경사각 계산 (도)
	 */
	float CalculateSlope(const FVector& Normal) const;

	/**
	 * 포인트를 지형에 투영
	 */
	bool ProjectToTerrain(UWorld* World, FVector& InOutLocation, FVector& OutNormal, float RaycastDistance) const;
};
