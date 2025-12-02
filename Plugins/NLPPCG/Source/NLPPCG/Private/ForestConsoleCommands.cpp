// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ForestPCGManager.h"
#include "ForestPCGManagerLibrary.h"
#include "PCGGraph.h"

/**
 * 콘솔 명령으로 숲 생성 테스트
 *
 * 사용법:
 *   nlp.forest "밀집된 소나무 숲"
 *   nlp.forest "넓은 참나무 숲"
 *   nlp.forest.clear  - 숲 제거
 */

static FAutoConsoleCommand GenerateForestCommand(
	TEXT("nlp.forest"),
	TEXT("Generate forest using natural language. Usage: nlp.forest \"dense pine forest\""),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Usage: nlp.forest \"your forest description\""));
			UE_LOG(LogTemp, Warning, TEXT("Example: nlp.forest \"밀집된 소나무 숲\""));
			return;
		}

		// 명령 결합
		FString Command = FString::Join(Args, TEXT(" "));
		Command = Command.TrimQuotes();  // 큰따옴표 제거

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("🌲 Console Command: nlp.forest"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("   Command: %s"), *Command);
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		// 현재 월드 가져오기
		UWorld* World = nullptr;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor || WorldContext.WorldType == EWorldType::PIE)
			{
				World = WorldContext.World();
				break;
			}
		}

		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No valid world found!"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("   World found: %s"), *World->GetName());

		// ForestPCGManager 찾기 또는 생성
		AForestPCGManager* Manager = nullptr;
		bool bIsNewManager = false;
		for (TActorIterator<AForestPCGManager> It(World); It; ++It)
		{
			Manager = *It;
			UE_LOG(LogTemp, Log, TEXT("   Using existing ForestPCGManager: %s"), *Manager->GetName());
			break;
		}

		if (!Manager)
		{
			UE_LOG(LogTemp, Warning, TEXT("   No ForestPCGManager found, creating new one..."));
			Manager = UForestPCGManagerLibrary::GetOrCreateForestPCGManager(World, FVector::ZeroVector);

			if (!Manager)
			{
				UE_LOG(LogTemp, Error, TEXT("❌ Failed to create ForestPCGManager!"));
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("   ✅ Created new ForestPCGManager: %s"), *Manager->GetName());
			bIsNewManager = true;
		}

		// 에디터 모드에서는 BeginPlay가 자동으로 호출되지 않으므로 항상 MCPClient 초기화 확인
		// 기존 매니저도 MCPClient가 없을 수 있으므로 초기화 필요
		if (Manager)
		{
			UE_LOG(LogTemp, Warning, TEXT("   🔧 Ensuring MCPClient is initialized for Editor mode..."));
			Manager->InitializeMCPClient();
		}

		// 명령 전송
		UE_LOG(LogTemp, Warning, TEXT("🚀 Sending command via GenerateForestFromNLP()..."));
		Manager->GenerateForestFromNLP(Command);

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("✅ Command sent! Check Output Log for results."));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	})
);

static FAutoConsoleCommand ClearForestCommand(
	TEXT("nlp.forest.clear"),
	TEXT("Clear all forests"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("🗑️  Clearing all forests..."));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		UWorld* World = nullptr;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor || WorldContext.WorldType == EWorldType::PIE)
			{
				World = WorldContext.World();
				break;
			}
		}

		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No valid world found!"));
			return;
		}

		int32 ClearedCount = 0;
		for (TActorIterator<AForestPCGManager> It(World); It; ++It)
		{
			AForestPCGManager* Manager = *It;
			Manager->ClearForest();
			ClearedCount++;
			UE_LOG(LogTemp, Log, TEXT("   Cleared forest: %s"), *Manager->GetName());
		}

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("✅ Cleared %d forest(s)"), ClearedCount);
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	})
);

static FAutoConsoleCommand ListForestsCommand(
	TEXT("nlp.forest.list"),
	TEXT("List all ForestPCGManagers in the level"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UWorld* World = nullptr;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor || WorldContext.WorldType == EWorldType::PIE)
			{
				World = WorldContext.World();
				break;
			}
		}

		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No valid world found!"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("📋 ForestPCGManagers in Level"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		int32 Count = 0;
		for (TActorIterator<AForestPCGManager> It(World); It; ++It)
		{
			AForestPCGManager* Manager = *It;
			Count++;

			UE_LOG(LogTemp, Log, TEXT("%d. %s"), Count, *Manager->GetName());
			UE_LOG(LogTemp, Log, TEXT("   Location: %s"), *Manager->GetActorLocation().ToString());
			UE_LOG(LogTemp, Log, TEXT("   MCPClient: %s"), Manager->MCPClient ? *Manager->MCPClient->GetName() : TEXT("NULL"));
			UE_LOG(LogTemp, Log, TEXT("   PCG Graph: %s"), Manager->PCGGraphAsset ? *Manager->PCGGraphAsset->GetName() : TEXT("None"));
		}

		if (Count == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("   No ForestPCGManagers found in level"));
			UE_LOG(LogTemp, Warning, TEXT("   Use: nlp.forest \"your command\" to create one"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("========================================"));
			UE_LOG(LogTemp, Warning, TEXT("✅ Found %d ForestPCGManager(s)"), Count);
		}

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	})
);

static FAutoConsoleCommand HelpForestCommand(
	TEXT("nlp.forest.help"),
	TEXT("Show help for forest commands"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("🌲 NLPPCG Forest Commands Help"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("Commands:"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("  nlp.forest <description>"));
		UE_LOG(LogTemp, Warning, TEXT("    Generate forest using natural language"));
		UE_LOG(LogTemp, Warning, TEXT("    Examples:"));
		UE_LOG(LogTemp, Warning, TEXT("      nlp.forest \"밀집된 소나무 숲\""));
		UE_LOG(LogTemp, Warning, TEXT("      nlp.forest \"넓은 참나무 숲\""));
		UE_LOG(LogTemp, Warning, TEXT("      nlp.forest \"작은 자작나무 숲\""));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("  nlp.forest.clear"));
		UE_LOG(LogTemp, Warning, TEXT("    Clear all forests in the level"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("  nlp.forest.list"));
		UE_LOG(LogTemp, Warning, TEXT("    List all ForestPCGManagers in the level"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("  nlp.forest.help"));
		UE_LOG(LogTemp, Warning, TEXT("    Show this help message"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("💡 Tip: Check the Output Log for detailed"));
		UE_LOG(LogTemp, Warning, TEXT("    information about forest generation"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	})
);
