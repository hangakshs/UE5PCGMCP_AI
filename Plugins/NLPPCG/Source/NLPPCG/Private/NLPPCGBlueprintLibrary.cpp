#include "NLPPCGBlueprintLibrary.h"
#include "ForestPCGManager.h"
#include "MCPClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
    template<typename TActor>
    TActor* FindActor(UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<TActor> It(World); It; ++It)
        {
            return *It;
        }

        return nullptr;
    }
}

bool UNLPPCGBlueprintLibrary::SendForestCommand(UObject* WorldContextObject, const FString& Command)
{
    AMCPClient* Client = GetMCPClient(WorldContextObject);
    if (!Client)
    {
        UE_LOG(LogTemp, Warning, TEXT("NLPPCGBlueprintLibrary: MCPClient not found"));
        return false;
    }

    Client->SendCommand(Command);
    return true;
}

bool UNLPPCGBlueprintLibrary::ClearAllForests(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    bool bCleared = false;
    for (TActorIterator<AForestPCGManager> It(World); It; ++It)
    {
        It->ClearForest();
        bCleared = true;
    }

    return bCleared;
}

AMCPClient* UNLPPCGBlueprintLibrary::GetMCPClient(UObject* WorldContextObject)
{
    return FindActor<AMCPClient>(WorldContextObject);
}

AForestPCGManager* UNLPPCGBlueprintLibrary::GetForestPCGManager(UObject* WorldContextObject)
{
    return FindActor<AForestPCGManager>(WorldContextObject);
}