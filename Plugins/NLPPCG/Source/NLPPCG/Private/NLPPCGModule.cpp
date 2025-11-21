// Copyright Epic Games, Inc. All Rights Reserved.

#include "NLPPCGModule.h"

#define LOCTEXT_NAMESPACE "FNLPPCGModule"

void FNLPPCGModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("NLPPCG Module Started"));
}

void FNLPPCGModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("NLPPCG Module Shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNLPPCGModule, NLPPCG)
