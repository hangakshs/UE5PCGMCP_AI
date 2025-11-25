// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNLPPCGModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** File Watcher Service 프로세스 핸들 */
	FProcHandle FileWatcherProcessHandle;

	/** File Watcher Service 시작 */
	void StartFileWatcherService();

	/** File Watcher Service 종료 */
	void StopFileWatcherService();
};
