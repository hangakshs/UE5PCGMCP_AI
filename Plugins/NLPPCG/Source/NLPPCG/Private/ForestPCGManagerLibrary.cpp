// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestPCGManagerLibrary.h"
#include "ForestPCGManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AForestPCGManager* UForestPCGManagerLibrary::GetOrCreateForestPCGManager(
	UObject* WorldContextObject,
	FVector SpawnLocation)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid World Context"));
		return nullptr;
	}

	// 레벨에서 기존 ForestPCGManager 찾기
	for (TActorIterator<AForestPCGManager> It(World); It; ++It)
	{
		AForestPCGManager* Manager = *It;
		if (Manager && !Manager->IsPendingKillPending())
		{
			UE_LOG(LogTemp, Log, TEXT("Found existing ForestPCGManager: %s"), *Manager->GetName());
			return Manager;
		}
	}

	// 없으면 새로 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("ForestPCGManager_Auto"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AForestPCGManager* NewManager = World->SpawnActor<AForestPCGManager>(
		AForestPCGManager::StaticClass(),
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewManager)
	{
		UE_LOG(LogTemp, Log, TEXT("Created new ForestPCGManager at location: %s"), *SpawnLocation.ToString());

		// 자동 설정
		AutoSetupForestPCGManager(NewManager);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create ForestPCGManager"));
	}

	return NewManager;
}

bool UForestPCGManagerLibrary::GenerateForestFromNLP(
	UObject* WorldContextObject,
	const FString& Command,
	FVector SpawnLocation)
{
	if (Command.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty command"));
		return false;
	}

	// ForestPCGManager 가져오거나 생성
	AForestPCGManager* Manager = GetOrCreateForestPCGManager(WorldContextObject, SpawnLocation);
	if (!Manager)
	{
		return false;
	}

	// 숲 생성
	Manager->GenerateForestFromNLP(Command);

	UE_LOG(LogTemp, Log, TEXT("GenerateForestFromNLP executed: %s"), *Command);
	return true;
}

int32 UForestPCGManagerLibrary::SaveAllPCGGraphsAsAssets(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return 0;
	}

	int32 SavedCount = 0;

	// 모든 ForestPCGManager 찾기
	for (TActorIterator<AForestPCGManager> It(World); It; ++It)
	{
		AForestPCGManager* Manager = *It;
		if (Manager && Manager->SavePCGGraphAsAsset())
		{
			SavedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Saved %d PCG Graph(s) as assets"), SavedCount);
	return SavedCount;
}

void UForestPCGManagerLibrary::AutoSetupForestPCGManager(AForestPCGManager* Manager)
{
	if (!Manager)
	{
		return;
	}

	// 나무 메시 초기화 (아직 안 되어 있다면)
	if (Manager->TreeMeshes.Num() == 0)
	{
		Manager->InitializeDefaultTreeMeshes();
		UE_LOG(LogTemp, Log, TEXT("Initialized tree meshes for %s"), *Manager->GetName());
	}

	// PCG 그래프 자동 저장 활성화
	Manager->bSavePCGGraphAsAsset = true;

	UE_LOG(LogTemp, Log, TEXT("Auto-setup completed for %s"), *Manager->GetName());
}
