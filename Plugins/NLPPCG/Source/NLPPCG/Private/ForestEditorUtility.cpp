// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestEditorUtility.h"
#include "ForestPCGManagerLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UForestEditorUtility::GenerateForestFromUI(const FString& Command)
{
	if (Command.IsEmpty())
	{
		LastResultMessage = TEXT("❌ 명령을 입력해주세요");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *LastResultMessage);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		LastResultMessage = TEXT("❌ 월드를 찾을 수 없습니다");
		UE_LOG(LogTemp, Error, TEXT("%s"), *LastResultMessage);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("🌲 Forest Editor Utility - Generate Forest"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("   Command: %s"), *Command);

	// ForestPCGManager 찾기 또는 생성
	bool bSuccess = UForestPCGManagerLibrary::GenerateForestFromNLP(
		World,
		Command,
		FVector::ZeroVector
	);

	if (bSuccess)
	{
		LastResultMessage = FString::Printf(TEXT("✅ 숲 생성 명령 전송: %s"), *Command);
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("✅ Command sent successfully!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	}
	else
	{
		LastResultMessage = TEXT("❌ 숲 생성 실패 - Output Log 확인");
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to send command"));
		UE_LOG(LogTemp, Error, TEXT("========================================"));
	}

	return bSuccess;
}

void UForestEditorUtility::ClearAllForests()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		LastResultMessage = TEXT("❌ 월드를 찾을 수 없습니다");
		UE_LOG(LogTemp, Error, TEXT("%s"), *LastResultMessage);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("🗑️  Clearing all forests..."));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));

	int32 ClearedCount = 0;
	for (TActorIterator<AForestPCGManager> It(World); It; ++It)
	{
		AForestPCGManager* Manager = *It;
		if (Manager)
		{
			Manager->ClearForest();
			ClearedCount++;
			UE_LOG(LogTemp, Log, TEXT("   Cleared: %s"), *Manager->GetName());
		}
	}

	LastResultMessage = FString::Printf(TEXT("✅ %d개의 숲 제거 완료"), ClearedCount);

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("✅ Cleared %d forest(s)"), ClearedCount);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

int32 UForestEditorUtility::GetForestManagerCount() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<AForestPCGManager> It(World); It; ++It)
	{
		Count++;
	}

	return Count;
}

TArray<FString> UForestEditorUtility::GetExampleCommands() const
{
	return {
		TEXT("밀집된 소나무 숲"),
		TEXT("넓은 참나무 숲"),
		TEXT("작은 자작나무 숲"),
		TEXT("희박한 단풍나무 숲"),
		TEXT("중간 크기의 소나무 숲"),
		TEXT("큰 참나무 숲"),
		TEXT("보통 자작나무 숲"),
		TEXT("dense pine forest"),
		TEXT("large oak forest"),
		TEXT("small birch forest")
	};
}
