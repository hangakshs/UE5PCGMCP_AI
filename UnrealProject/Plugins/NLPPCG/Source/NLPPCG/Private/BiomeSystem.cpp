// Copyright Epic Games, Inc. All Rights Reserved.

#include "BiomeSystem.h"

UBiomeDataAsset::UBiomeDataAsset()
{
	CreateDefaultBiomes();
}

FBiomeSettings UBiomeDataAsset::GetBiomeSettings(EBiomeType BiomeType) const
{
	for (const FBiomeSettings& Biome : Biomes)
	{
		if (Biome.BiomeType == BiomeType)
		{
			return Biome;
		}
	}

	// 기본값 반환
	return FBiomeSettings();
}

FBiomeSettings UBiomeDataAsset::GetBiomeByName(const FString& BiomeName) const
{
	for (const FBiomeSettings& Biome : Biomes)
	{
		if (Biome.BiomeName.Equals(BiomeName, ESearchCase::IgnoreCase))
		{
			return Biome;
		}
	}

	return FBiomeSettings();
}

FString UBiomeDataAsset::SelectRandomTreeSpecies(const FBiomeSettings& Biome, FRandomStream& RandomStream) const
{
	if (Biome.SpeciesDistribution.Num() == 0)
	{
		return TEXT("generic_tree");
	}

	// 비율 기반 선택
	float TotalProportion = 0.0f;
	for (const FTreeSpeciesDistribution& Species : Biome.SpeciesDistribution)
	{
		TotalProportion += Species.Proportion;
	}

	float RandomValue = RandomStream.FRandRange(0.0f, TotalProportion);
	float CurrentProportion = 0.0f;

	for (const FTreeSpeciesDistribution& Species : Biome.SpeciesDistribution)
	{
		CurrentProportion += Species.Proportion;
		if (RandomValue <= CurrentProportion)
		{
			return Species.TreeType;
		}
	}

	return Biome.SpeciesDistribution[0].TreeType;
}

void UBiomeDataAsset::CreateDefaultBiomes()
{
	Biomes.Empty();

	// 1. 침엽수림 (Coniferous Forest)
	{
		FBiomeSettings Biome;
		Biome.BiomeType = EBiomeType::ConiferousForest;
		Biome.BiomeName = TEXT("Coniferous Forest");

		FTreeSpeciesDistribution Pine;
		Pine.TreeType = TEXT("pine");
		Pine.Proportion = 0.7f;
		Pine.bDominantSpecies = true;
		Biome.SpeciesDistribution.Add(Pine);

		FTreeSpeciesDistribution GenericTree;
		GenericTree.TreeType = TEXT("generic_tree");
		GenericTree.Proportion = 0.3f;
		Biome.SpeciesDistribution.Add(GenericTree);

		Biome.BaseParameters.Density = TEXT("dense");
		Biome.BaseParameters.MinDistance = 200.0f;
		Biome.BaseParameters.MaxDistance = 400.0f;

		Biome.MinTemperature = -20.0f;
		Biome.MaxTemperature = 15.0f;
		Biome.AnnualPrecipitation = 500.0f;

		Biomes.Add(Biome);
	}

	// 2. 활엽수림 (Deciduous Forest)
	{
		FBiomeSettings Biome;
		Biome.BiomeType = EBiomeType::DeciduousForest;
		Biome.BiomeName = TEXT("Deciduous Forest");

		FTreeSpeciesDistribution Oak;
		Oak.TreeType = TEXT("oak");
		Oak.Proportion = 0.4f;
		Oak.bDominantSpecies = true;
		Biome.SpeciesDistribution.Add(Oak);

		FTreeSpeciesDistribution Maple;
		Maple.TreeType = TEXT("maple");
		Maple.Proportion = 0.3f;
		Biome.SpeciesDistribution.Add(Maple);

		FTreeSpeciesDistribution Birch;
		Birch.TreeType = TEXT("birch");
		Birch.Proportion = 0.3f;
		Biome.SpeciesDistribution.Add(Birch);

		Biome.BaseParameters.Density = TEXT("medium");
		Biome.BaseParameters.MinDistance = 250.0f;
		Biome.BaseParameters.MaxDistance = 500.0f;

		Biome.bEnableStratification = true;
		Biome.CanopyProportion = 0.6f;
		Biome.UnderstoryProportion = 0.3f;
		Biome.ShrubProportion = 0.1f;

		Biome.MinTemperature = -5.0f;
		Biome.MaxTemperature = 25.0f;
		Biome.AnnualPrecipitation = 800.0f;

		Biomes.Add(Biome);
	}

	// 3. 혼합림 (Mixed Forest)
	{
		FBiomeSettings Biome;
		Biome.BiomeType = EBiomeType::MixedForest;
		Biome.BiomeName = TEXT("Mixed Forest");

		FTreeSpeciesDistribution Pine;
		Pine.TreeType = TEXT("pine");
		Pine.Proportion = 0.25f;
		Biome.SpeciesDistribution.Add(Pine);

		FTreeSpeciesDistribution Oak;
		Oak.TreeType = TEXT("oak");
		Oak.Proportion = 0.25f;
		Biome.SpeciesDistribution.Add(Oak);

		FTreeSpeciesDistribution Birch;
		Birch.TreeType = TEXT("birch");
		Birch.Proportion = 0.25f;
		Biome.SpeciesDistribution.Add(Birch);

		FTreeSpeciesDistribution Maple;
		Maple.TreeType = TEXT("maple");
		Maple.Proportion = 0.25f;
		Biome.SpeciesDistribution.Add(Maple);

		Biome.BaseParameters.Density = TEXT("medium");
		Biome.BaseParameters.MinDistance = 225.0f;
		Biome.BaseParameters.MaxDistance = 450.0f;

		Biome.bEnableStratification = true;
		Biome.CanopyProportion = 0.65f;
		Biome.UnderstoryProportion = 0.25f;
		Biome.ShrubProportion = 0.1f;

		Biome.MinTemperature = -10.0f;
		Biome.MaxTemperature = 20.0f;
		Biome.AnnualPrecipitation = 700.0f;

		Biomes.Add(Biome);
	}

	// 4. 타이가 (Taiga)
	{
		FBiomeSettings Biome;
		Biome.BiomeType = EBiomeType::Taiga;
		Biome.BiomeName = TEXT("Taiga");

		FTreeSpeciesDistribution Pine;
		Pine.TreeType = TEXT("pine");
		Pine.Proportion = 0.9f;
		Pine.bDominantSpecies = true;
		Biome.SpeciesDistribution.Add(Pine);

		FTreeSpeciesDistribution Birch;
		Birch.TreeType = TEXT("birch");
		Birch.Proportion = 0.1f;
		Biome.SpeciesDistribution.Add(Birch);

		Biome.BaseParameters.Density = TEXT("medium");
		Biome.BaseParameters.MinDistance = 300.0f;
		Biome.BaseParameters.MaxDistance = 600.0f;

		Biome.MinTemperature = -40.0f;
		Biome.MaxTemperature = 10.0f;
		Biome.AnnualPrecipitation = 400.0f;

		Biomes.Add(Biome);
	}
}

bool UBiomeForestGenerator::GenerateParametersFromBiome(
	const FBiomeSettings& BiomeSettings,
	TArray<FPCGForestParameters>& OutParameters)
{
	OutParameters.Empty();

	if (!BiomeSettings.bEnableStratification)
	{
		// 단일 층 - 기본 파라미터만 사용
		OutParameters.Add(BiomeSettings.BaseParameters);
		return true;
	}

	// 층위 구조 (Stratification)
	// 1. 교목층 (Canopy)
	if (BiomeSettings.CanopyProportion > 0.0f)
	{
		FPCGForestParameters CanopyParams = BiomeSettings.BaseParameters;
		CanopyParams.ScaleMultiplier = 1.5f; // 큰 나무
		CanopyParams.MinDistance = BiomeSettings.BaseParameters.MinDistance * 1.5f;
		CanopyParams.MaxDistance = BiomeSettings.BaseParameters.MaxDistance * 1.5f;
		OutParameters.Add(CanopyParams);
	}

	// 2. 아교목층 (Understory)
	if (BiomeSettings.UnderstoryProportion > 0.0f)
	{
		FPCGForestParameters UnderstoryParams = BiomeSettings.BaseParameters;
		UnderstoryParams.ScaleMultiplier = 0.8f; // 중간 크기
		UnderstoryParams.MinDistance = BiomeSettings.BaseParameters.MinDistance * 0.8f;
		UnderstoryParams.MaxDistance = BiomeSettings.BaseParameters.MaxDistance * 0.8f;
		OutParameters.Add(UnderstoryParams);
	}

	// 3. 관목층 (Shrub)
	if (BiomeSettings.ShrubProportion > 0.0f)
	{
		FPCGForestParameters ShrubParams = BiomeSettings.BaseParameters;
		ShrubParams.ScaleMultiplier = 0.4f; // 작은 나무/관목
		ShrubParams.MinDistance = BiomeSettings.BaseParameters.MinDistance * 0.5f;
		ShrubParams.MaxDistance = BiomeSettings.BaseParameters.MaxDistance * 0.5f;
		OutParameters.Add(ShrubParams);
	}

	return OutParameters.Num() > 0;
}

EBiomeType UBiomeForestGenerator::FindSuitableBiome(
	float Temperature,
	float Precipitation,
	const UBiomeDataAsset* BiomeData)
{
	if (!BiomeData)
	{
		return EBiomeType::MixedForest;
	}

	// 기후 조건에 가장 적합한 바이옴 찾기
	EBiomeType BestBiome = EBiomeType::MixedForest;
	float BestScore = -1.0f;

	for (const FBiomeSettings& Biome : BiomeData->Biomes)
	{
		// 온도 적합도
		float TempScore = 0.0f;
		if (Temperature >= Biome.MinTemperature && Temperature <= Biome.MaxTemperature)
		{
			// 범위 내: 중간값에 가까울수록 높은 점수
			float MidTemp = (Biome.MinTemperature + Biome.MaxTemperature) * 0.5f;
			float TempRange = Biome.MaxTemperature - Biome.MinTemperature;
			TempScore = 1.0f - FMath::Abs(Temperature - MidTemp) / (TempRange * 0.5f);
		}

		// 강수량 적합도 (±30% 허용)
		float PrecipScore = 0.0f;
		float PrecipDiff = FMath::Abs(Precipitation - Biome.AnnualPrecipitation);
		float PrecipTolerance = Biome.AnnualPrecipitation * 0.3f;
		if (PrecipDiff <= PrecipTolerance)
		{
			PrecipScore = 1.0f - (PrecipDiff / PrecipTolerance);
		}

		// 총점
		float TotalScore = (TempScore + PrecipScore) * 0.5f;

		if (TotalScore > BestScore)
		{
			BestScore = TotalScore;
			BestBiome = Biome.BiomeType;
		}
	}

	return BestBiome;
}
