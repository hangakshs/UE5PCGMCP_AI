// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreeMeshLibrary.h"

TArray<FTreeMeshEntry> UTreeMeshLibrary::GetMeshesByType(const FString& TreeType, const FString& Season) const
{
	TArray<FTreeMeshEntry> Result;

	for (const FTreeMeshEntry& Entry : TreeMeshes)
	{
		// 타입 매칭
		if (Entry.TreeType != TreeType)
		{
			continue;
		}

		// 계절 필터 (비어있거나 "All"이면 모든 계절)
		if (!Season.IsEmpty() && Entry.Season != TEXT("All") && Entry.Season != Season)
		{
			continue;
		}

		Result.Add(Entry);
	}

	return Result;
}

UStaticMesh* UTreeMeshLibrary::GetRandomMesh(const FString& TreeType, FRandomStream& RandomStream) const
{
	TArray<FTreeMeshEntry> Candidates = GetMeshesByType(TreeType);

	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No meshes found for tree type: %s"), *TreeType);
		return nullptr;
	}

	// 무게 기반 선택
	float TotalWeight = 0.0f;
	for (const FTreeMeshEntry& Entry : Candidates)
	{
		TotalWeight += Entry.Weight;
	}

	float RandomValue = RandomStream.FRandRange(0.0f, TotalWeight);
	float CurrentWeight = 0.0f;

	for (const FTreeMeshEntry& Entry : Candidates)
	{
		CurrentWeight += Entry.Weight;
		if (RandomValue <= CurrentWeight)
		{
			// 해당 엔트리의 메시 중 랜덤 선택
			if (Entry.Meshes.Num() > 0)
			{
				int32 MeshIndex = RandomStream.RandRange(0, Entry.Meshes.Num() - 1);
				return Entry.Meshes[MeshIndex].LoadSynchronous();
			}
		}
	}

	// 폴백: 첫 번째 메시
	if (Candidates[0].Meshes.Num() > 0)
	{
		return Candidates[0].Meshes[0].LoadSynchronous();
	}

	return nullptr;
}

void UTreeMeshLibrary::GetScaleInfo(const FString& TreeType, FVector& OutDefaultScale, FVector& OutScaleVariation) const
{
	TArray<FTreeMeshEntry> Meshes = GetMeshesByType(TreeType);

	if (Meshes.Num() > 0)
	{
		OutDefaultScale = Meshes[0].DefaultScale;
		OutScaleVariation = Meshes[0].ScaleVariation;
	}
	else
	{
		OutDefaultScale = FVector(1.0f, 1.0f, 1.0f);
		OutScaleVariation = FVector(0.2f, 0.2f, 0.2f);
	}
}
