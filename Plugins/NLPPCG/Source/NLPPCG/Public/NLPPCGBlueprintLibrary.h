// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NLPPCGBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for NLPPCG
 * Provides utility functions for forest generation
 */
UCLASS(Blueprintable, BlueprintType)
class NLPPCG_API UNLPPCGBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Send a forest generation command to MCP
	 * @param Command Natural language command (e.g., "밀집된 소나무 숲")
	 * @return True if command was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static bool SendForestCommand(UObject* WorldContextObject, const FString& Command);

	/**
	 * Clear all forests in the level
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static bool ClearAllForests(UObject* WorldContextObject);

	/**
	 * Get the MCPClient actor in the level
	 * @return MCPClient actor or nullptr if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static class AMCPClient* GetMCPClient(UObject* WorldContextObject);

	/**
	 * Get the ForestPCGManager actor in the level
	 * @return ForestPCGManager actor or nullptr if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "NLPPCG", meta = (WorldContext = "WorldContextObject"))
	static class AForestPCGManager* GetForestPCGManager(UObject* WorldContextObject);
};
