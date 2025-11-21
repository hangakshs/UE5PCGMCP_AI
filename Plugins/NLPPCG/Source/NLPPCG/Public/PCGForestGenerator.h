// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGForestGenerator.generated.h"

/**
 * PCG 숲 생성 파라미터
 */
USTRUCT(BlueprintType)
struct FPCGForestParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	FString TreeType = TEXT("generic_tree");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	FString Density = TEXT("medium");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	FString Size = TEXT("medium");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	float AreaSize = 5000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	float MinDistance = 200.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	float MaxDistance = 500.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	float Randomness = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Forest")
	float ScaleMultiplier = 1.0f;
};

/**
 * PCG 노드: 숲 포인트 생성
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class NLPPCG_API UPCGForestGeneratorSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGForestGeneratorSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("ForestGenerator")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGForestGenerator", "NodeTitle", "Forest Generator"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	/** 숲 생성 파라미터 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FPCGForestParameters ForestParameters;

	/** 경계 영역 크기 (cm) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FVector BoundsSize = FVector(7071.0f, 7071.0f, 1000.0f); // 50m x 50m default

	// Note: Seed는 부모 클래스 UPCGSettings에 이미 정의되어 있습니다
};

/**
 * PCG 엘리먼트: 숲 포인트 생성 로직
 */
class FPCGForestGeneratorElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
