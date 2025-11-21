// Copyright Epic Games, Inc. All Rights Reserved.

#include "NLPPCGWorldSubsystem.h"
#include "MCPClient.h"
#include "ForestPCGManager.h"
#include "PCGForestGenerator.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UNLPPCGWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::Initialize - Subsystem initialized"));

	// Bind to world begin play event
	if (UWorld* World = GetWorld())
	{
		// For Editor worlds, initialize immediately
		#if WITH_EDITOR
		if (World->WorldType == EWorldType::Editor)
		{
			UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::Initialize - Editor world detected, initializing immediately"));
			OnWorldBeginPlay();
		}
		else
		#endif
		{
			// For game worlds, wait for begin play
			FWorldDelegates::OnWorldBeginPlay.AddUObject(this, &UNLPPCGWorldSubsystem::OnWorldBeginPlay);
		}
	}
}

void UNLPPCGWorldSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::Deinitialize - Subsystem shutting down"));

	// Cleanup
	MCPClientInstance = nullptr;
	ForestPCGManagerInstance = nullptr;

	Super::Deinitialize();
}

void UNLPPCGWorldSubsystem::OnWorldBeginPlay()
{
	UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::OnWorldBeginPlay - Auto-initializing NLPPCG system"));

	// Auto-create MCP Client and Forest PCG Manager
	GetOrCreateMCPClient();
	GetOrCreateForestPCGManager();

	// Bind them together
	BindMCPClientToManager();

	UE_LOG(LogTemp, Warning, TEXT("=== NLPPCG System Ready ==="));
	UE_LOG(LogTemp, Warning, TEXT("MCP Client and Forest PCG Manager have been automatically initialized."));
	UE_LOG(LogTemp, Warning, TEXT("You can now use the MCP server to generate forests!"));
	UE_LOG(LogTemp, Warning, TEXT("Example command: '밀집된 소나무 숲'"));
}

AMCPClient* UNLPPCGWorldSubsystem::GetOrCreateMCPClient()
{
	if (MCPClientInstance && MCPClientInstance->IsValidLowLevel())
	{
		return MCPClientInstance;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::GetOrCreateMCPClient - No valid world"));
		return nullptr;
	}

	// Search for existing MCP Client in the level
	for (TActorIterator<AMCPClient> It(World); It; ++It)
	{
		MCPClientInstance = *It;
		UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::GetOrCreateMCPClient - Found existing MCP Client"));
		return MCPClientInstance;
	}

	// Create new MCP Client
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("AutoMCPClient"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	MCPClientInstance = World->SpawnActor<AMCPClient>(AMCPClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (MCPClientInstance)
	{
		// Enable debug mode
		MCPClientInstance->bDebugMode = true;
		UE_LOG(LogTemp, Warning, TEXT("UNLPPCGWorldSubsystem::GetOrCreateMCPClient - Created new MCP Client (Debug Mode: ON)"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::GetOrCreateMCPClient - Failed to create MCP Client"));
	}

	return MCPClientInstance;
}

AForestPCGManager* UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager()
{
	if (ForestPCGManagerInstance && ForestPCGManagerInstance->IsValidLowLevel())
	{
		return ForestPCGManagerInstance;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager - No valid world"));
		return nullptr;
	}

	// Search for existing Forest PCG Manager in the level
	for (TActorIterator<AForestPCGManager> It(World); It; ++It)
	{
		ForestPCGManagerInstance = *It;
		UE_LOG(LogTemp, Log, TEXT("UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager - Found existing Forest PCG Manager"));
		return ForestPCGManagerInstance;
	}

	// Create new Forest PCG Manager
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("AutoForestPCGManager"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ForestPCGManagerInstance = World->SpawnActor<AForestPCGManager>(
		AForestPCGManager::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (ForestPCGManagerInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager - Created new Forest PCG Manager"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager - Failed to create Forest PCG Manager"));
	}

	return ForestPCGManagerInstance;
}

void UNLPPCGWorldSubsystem::BindMCPClientToManager()
{
	if (!MCPClientInstance || !ForestPCGManagerInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::BindMCPClientToManager - MCP Client or Forest PCG Manager is null"));
		return;
	}

	// Bind the OnForestGenerated delegate to the manager's GenerateForest function
	if (!MCPClientInstance->OnForestGenerated.IsBoundToObject(ForestPCGManagerInstance))
	{
		MCPClientInstance->OnForestGenerated.AddDynamic(ForestPCGManagerInstance, &AForestPCGManager::GenerateForest);
		UE_LOG(LogTemp, Warning, TEXT("UNLPPCGWorldSubsystem::BindMCPClientToManager - MCP Client bound to Forest PCG Manager"));
	}

	// Also bind to this subsystem for logging
	if (!MCPClientInstance->OnForestGenerated.IsBoundToObject(this))
	{
		MCPClientInstance->OnForestGenerated.AddDynamic(this, &UNLPPCGWorldSubsystem::OnForestGeneratedHandler);
	}
}

void UNLPPCGWorldSubsystem::OnForestGeneratedHandler(FPCGForestParameters Parameters)
{
	UE_LOG(LogTemp, Warning, TEXT("=== Forest Generation Triggered ==="));
	UE_LOG(LogTemp, Warning, TEXT("Tree Type: %s"), *Parameters.TreeType);
	UE_LOG(LogTemp, Warning, TEXT("Density: %s"), *Parameters.Density);
	UE_LOG(LogTemp, Warning, TEXT("Area Size: %.1f"), Parameters.AreaSize);
	UE_LOG(LogTemp, Warning, TEXT("Min Distance: %.1f"), Parameters.MinDistance);
	UE_LOG(LogTemp, Warning, TEXT("Max Distance: %.1f"), Parameters.MaxDistance);
}

void UNLPPCGWorldSubsystem::TestForestGeneration(const FString& Command)
{
	if (!MCPClientInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UNLPPCGWorldSubsystem::TestForestGeneration - MCP Client not initialized"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("UNLPPCGWorldSubsystem::TestForestGeneration - Sending test command: %s"), *Command);
	MCPClientInstance->SendCommand(Command);
}
