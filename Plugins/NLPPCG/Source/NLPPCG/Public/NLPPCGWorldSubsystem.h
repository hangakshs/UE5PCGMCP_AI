// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NLPPCGWorldSubsystem.generated.h"

class AMCPClient;
class AForestPCGManager;

/**
 * World Subsystem that automatically initializes MCP Client and Forest PCG Manager
 * when a level is loaded in the editor or game.
 */
UCLASS()
class NLPPCG_API UNLPPCGWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem implementation
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Get or create the MCP Client instance
	UFUNCTION(BlueprintCallable, Category = "NLPPCG")
	AMCPClient* GetOrCreateMCPClient();

	// Get or create the Forest PCG Manager instance
	UFUNCTION(BlueprintCallable, Category = "NLPPCG")
	AForestPCGManager* GetOrCreateForestPCGManager();

	// Test forest generation
	UFUNCTION(BlueprintCallable, Category = "NLPPCG")
	void TestForestGeneration(const FString& Command);

private:
	// Cached instances
	UPROPERTY()
	TObjectPtr<AMCPClient> MCPClientInstance;

	UPROPERTY()
	TObjectPtr<AForestPCGManager> ForestPCGManagerInstance;

	// Auto-initialize on world begin play
	void OnWorldBeginPlay();

	// Bind MCP Client to Forest PCG Manager
	void BindMCPClientToManager();

	// Handler for forest generation delegate
	UFUNCTION()
	void OnForestGeneratedHandler(FPCGForestParameters Parameters);
};
