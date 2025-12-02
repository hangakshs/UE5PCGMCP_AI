// Copyright Epic Games, Inc. All Rights Reserved.

#include "NLPPCGModule.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FNLPPCGModule"

void FNLPPCGModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("NLPPCG Module Started"));

	// File Watcher Service 자동 시작
	StartFileWatcherService();
}

void FNLPPCGModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("NLPPCG Module Shutdown"));

	// File Watcher Service 자동 종료
	StopFileWatcherService();
}

void FNLPPCGModule::StartFileWatcherService()
{
	// 프로젝트 루트 경로 가져오기
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	// MCPServer 경로 구성 (프로젝트 내부에 위치)
	FString MCPServerDir = FPaths::Combine(ProjectDir, TEXT("MCPServer"));
	MCPServerDir = FPaths::ConvertRelativePathToFull(MCPServerDir);

	// Python 스크립트 경로
	FString PythonScript = FPaths::Combine(MCPServerDir, TEXT("src/file_watcher_service.py"));

	// Python 실행 파일 경로 찾기
	FString PythonExecutable;

#if PLATFORM_WINDOWS
	// Windows: python 또는 python3
	PythonExecutable = TEXT("python");
#else
	// Linux/Mac: python3
	PythonExecutable = TEXT("python3");
#endif

	// 명령행 인수 구성
	FString Arguments = FString::Printf(TEXT("\"%s\" --project-root \"%s\""),
		*PythonScript,
		*ProjectDir);

	UE_LOG(LogTemp, Log, TEXT("🚀 Starting File Watcher Service..."));
	UE_LOG(LogTemp, Log, TEXT("   Python: %s"), *PythonExecutable);
	UE_LOG(LogTemp, Log, TEXT("   Script: %s"), *PythonScript);
	UE_LOG(LogTemp, Log, TEXT("   Project Root: %s"), *ProjectDir);
	UE_LOG(LogTemp, Log, TEXT("   Arguments: %s"), *Arguments);

	// 프로세스 시작
	uint32 ProcessID = 0;
	FileWatcherProcessHandle = FPlatformProcess::CreateProc(
		*PythonExecutable,
		*Arguments,
		false,  // bLaunchDetached
		false,  // bLaunchHidden
		false,  // bLaunchReallyHidden
		&ProcessID,
		0,      // PriorityModifier
		*MCPServerDir,  // WorkingDirectory
		nullptr  // PipeWriteChild
	);

	if (FileWatcherProcessHandle.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("✅ File Watcher Service started (PID: %d)"), ProcessID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to start File Watcher Service"));
		UE_LOG(LogTemp, Error, TEXT("   Please check if Python is installed and in PATH"));
	}
}

void FNLPPCGModule::StopFileWatcherService()
{
	if (FileWatcherProcessHandle.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("⏹️  Stopping File Watcher Service..."));

		// 프로세스 종료
		FPlatformProcess::TerminateProc(FileWatcherProcessHandle, true);
		FPlatformProcess::CloseProc(FileWatcherProcessHandle);

		UE_LOG(LogTemp, Log, TEXT("✅ File Watcher Service stopped"));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNLPPCGModule, NLPPCG)
