// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ForestPCGManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

/**
 * 시스템 진단 명령
 * nlp.forest.diagnose - NLPPCG 시스템 상태 확인
 */

static FAutoConsoleCommand DiagnoseCommand(
	TEXT("nlp.forest.diagnose"),
	TEXT("Diagnose NLPPCG system status"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("🔍 NLPPCG System Diagnostics"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		// 1. World 체크
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
			UE_LOG(LogTemp, Warning, TEXT("========================================"));
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("✅ World: %s"), *World->GetName());

		// 2. ForestPCGManager 체크
		AForestPCGManager* Manager = nullptr;
		int32 ManagerCount = 0;
		for (TActorIterator<AForestPCGManager> It(World); It; ++It)
		{
			ManagerCount++;
			if (!Manager) Manager = *It;
		}

		if (ManagerCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️  No ForestPCGManager found in level"));
			UE_LOG(LogTemp, Warning, TEXT("   Will be created automatically when needed"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("✅ ForestPCGManagers: %d"), ManagerCount);
			if (Manager)
			{
				UE_LOG(LogTemp, Log, TEXT("   Manager: %s"), *Manager->GetName());

				// 3. MCPClient 체크
				if (Manager->MCPClient)
				{
					UE_LOG(LogTemp, Warning, TEXT("✅ MCPClient: %s"), *Manager->MCPClient->GetName());
					UE_LOG(LogTemp, Log, TEXT("   Server URL: %s"), *Manager->MCPClient->ServerURL);
					UE_LOG(LogTemp, Log, TEXT("   Use File Communication: %s"),
						Manager->MCPClient->bUseFileCommunication ? TEXT("YES") : TEXT("NO"));
					UE_LOG(LogTemp, Log, TEXT("   Debug Mode: %s"),
						Manager->MCPClient->bDebugMode ? TEXT("YES") : TEXT("NO"));
					UE_LOG(LogTemp, Log, TEXT("   Can Ever Tick: %s"),
						Manager->MCPClient->PrimaryActorTick.bCanEverTick ? TEXT("YES") : TEXT("NO"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("❌ MCPClient is NULL!"));
					UE_LOG(LogTemp, Warning, TEXT("   Try restarting the editor"));
				}

				// 4. PCG Component 체크
				if (Manager->PCGComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("✅ PCG Component found"));
					UE_LOG(LogTemp, Log, TEXT("   Graph: %s"),
						Manager->PCGComponent->GetGraph() ? TEXT("Set") : TEXT("Not set (will be created)"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("❌ PCG Component is NULL!"));
				}
			}
		}

		// 5. 파일 시스템 체크
		FString CommandDir = FPaths::ProjectIntermediateDir() / TEXT("MCP_Commands");
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (PlatformFile.DirectoryExists(*CommandDir))
		{
			UE_LOG(LogTemp, Warning, TEXT("✅ Command Directory exists"));
			UE_LOG(LogTemp, Log, TEXT("   Path: %s"), *CommandDir);

			// 파일 목록 확인
			TArray<FString> FoundFiles;
			PlatformFile.FindFiles(FoundFiles, *CommandDir, nullptr);

			if (FoundFiles.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("   Files found: %d"), FoundFiles.Num());
				for (const FString& File : FoundFiles)
				{
					FString FileName = FPaths::GetCleanFilename(File);
					int64 FileSize = PlatformFile.FileSize(*File);
					UE_LOG(LogTemp, Log, TEXT("      - %s (%lld bytes)"), *FileName, FileSize);
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("   No files in directory (normal when idle)"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ Command Directory does NOT exist!"));
			UE_LOG(LogTemp, Error, TEXT("   Expected: %s"), *CommandDir);
			UE_LOG(LogTemp, Warning, TEXT("   Trying to create..."));

			if (PlatformFile.CreateDirectoryTree(*CommandDir))
			{
				UE_LOG(LogTemp, Warning, TEXT("   ✅ Directory created successfully!"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("   ❌ Failed to create directory!"));
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("💡 Diagnosis Complete!"));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("   If everything shows ✅, try:"));
		UE_LOG(LogTemp, Warning, TEXT("   nlp.forest \"밀집된 소나무 숲\""));
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("   For more info:"));
		UE_LOG(LogTemp, Warning, TEXT("   nlp.forest.help"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	})
);
