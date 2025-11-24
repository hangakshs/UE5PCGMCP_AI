// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestPCGManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

AForestPCGManager::AForestPCGManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root Scene Component 생성
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Box Component 생성 (PCG Bounds용)
	BoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("PCGBounds"));
	BoundsComponent->SetupAttachment(RootComponent);
	// 기본 크기: 100m x 100m x 10m (10000cm x 10000cm x 1000cm)
	BoundsComponent->SetBoxExtent(FVector(5000.0f, 5000.0f, 500.0f));
	BoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsComponent->SetHiddenInGame(true);

	// PCG 컴포넌트 생성 및 설정
	// Note: PCGComponent는 ActorComponent이므로 SetupAttachment 불가
	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));

	// PCG Component 기본 설정
	if (PCGComponent)
	{
		PCGComponent->bActivated = true;
		PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
	}

	// 기본 메시 로드 (하위 호환성)
	LoadDefaultTreeMesh();

	// 나무 타입별 기본 메시 초기화
	InitializeDefaultTreeMeshes();
}

void AForestPCGManager::BeginPlay()
{
	Super::BeginPlay();

	// MCP 클라이언트 초기화 (게임 모드)
	InitializeMCPClient();
}

void AForestPCGManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 액터 배치 시 기본 설정
	if (!TreeMesh)
	{
		LoadDefaultTreeMesh();
	}

	// 나무 메시 맵 초기화 (비어있을 경우)
	if (TreeMeshes.Num() == 0)
	{
		InitializeDefaultTreeMeshes();
	}

	// PCG Graph Asset이 설정되어 있으면 사용
	if (PCGGraphAsset && PCGComponent)
	{
		PCGComponent->SetGraph(PCGGraphAsset);
		UE_LOG(LogTemp, Log, TEXT("Using existing PCG Graph Asset"));
	}

	// MCP 클라이언트 초기화 (에디터 모드)
	#if WITH_EDITOR
	if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
	{
		InitializeMCPClient();
	}
	#endif
}

void AForestPCGManager::GenerateForestFromNLP(const FString& Command)
{
	if (!MCPClient)
	{
		UE_LOG(LogTemp, Error, TEXT("MCP Client not found!"));
		return;
	}

	MCPClient->SendCommand(Command);
}

void AForestPCGManager::ClearForest()
{
	if (PCGComponent)
	{
		PCGComponent->Cleanup();
		UE_LOG(LogTemp, Log, TEXT("Forest cleared"));
	}
}

void AForestPCGManager::GenerateForestFromParameters(const FPCGForestParameters& Parameters)
{
	SetupPCGGraph(Parameters);
}

void AForestPCGManager::OnForestParametersReceived(const FPCGForestParameters& Parameters)
{
	UE_LOG(LogTemp, Log, TEXT("Received forest parameters from MCP"));

	// 숲 제거 시그널 (AreaSize가 0)
	if (Parameters.AreaSize <= 0.0f)
	{
		ClearForest();
		return;
	}

	SetupPCGGraph(Parameters);
}

void AForestPCGManager::SetupPCGGraph(const FPCGForestParameters& Parameters)
{
	if (!PCGComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("PCG Component not found!"));
		return;
	}

	// 영역 크기 계산 (AreaSize는 cm² 단위)
	float SideLength = FMath::Sqrt(Parameters.AreaSize);

	// BoundsComponent 크기 업데이트
	if (BoundsComponent)
	{
		// Extent는 중심에서의 거리이므로 절반
		float HalfSide = SideLength / 2.0f;
		BoundsComponent->SetBoxExtent(FVector(HalfSide, HalfSide, 500.0f));
		UE_LOG(LogTemp, Log, TEXT("Updated BoundsComponent: Extent=(%.1f, %.1f, 500)"), HalfSide, HalfSide);
	}

	// PCG 그래프 생성 또는 가져오기
	UPCGGraph* PCGGraph = PCGComponent->GetGraph();
	if (!PCGGraph)
	{
		PCGGraph = NewObject<UPCGGraph>(PCGComponent);
		PCGComponent->SetGraph(PCGGraph);
	}

	// 기존 노드 제거
	const TArray<UPCGNode*>& Nodes = PCGGraph->GetNodes();
	if (Nodes.Num() > 0)
	{
		TArray<UPCGNode*> NodesToRemove = Nodes;
		PCGGraph->RemoveNodes(NodesToRemove);
	}

	// 1. Forest Generator 노드 생성
	UPCGForestGeneratorSettings* ForestSettings = NewObject<UPCGForestGeneratorSettings>(PCGGraph);
	ForestSettings->ForestParameters = Parameters;
	ForestSettings->BoundsSize = FVector(SideLength, SideLength, 1000.0f);

	UPCGNode* ForestNode = PCGGraph->AddNode(ForestSettings);

	// 2. Static Mesh Spawner 노드 생성
	UPCGStaticMeshSpawnerSettings* SpawnerSettings = NewObject<UPCGStaticMeshSpawnerSettings>(PCGGraph);

	// UE5.7 메시 설정 - MeshSelectorWeighted 사용
	if (TreeMesh)
	{
		// MeshSelectorWeighted 생성
		UPCGMeshSelectorWeighted* MeshSelector = NewObject<UPCGMeshSelectorWeighted>(SpawnerSettings);

		// Mesh Entry 생성 및 설정
		FPCGMeshSelectorWeightedEntry MeshEntry;
		MeshEntry.Weight = 100;  // 가중치
		MeshEntry.bOverrideCollisionProfile = false;
		MeshEntry.Mesh = TSoftObjectPtr<UStaticMesh>(TreeMesh);

		// Mesh Selector에 Entry 추가
		MeshSelector->MeshEntries.Add(MeshEntry);

		// Spawner Settings에 Mesh Selector 할당
		SpawnerSettings->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
		SpawnerSettings->MeshSelectorParameters = MeshSelector;

		UE_LOG(LogTemp, Log, TEXT("Configured Static Mesh Spawner with mesh: %s"), *TreeMesh->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Tree mesh not set! Forest generation will fail."));
	}

	// 스폰 옵션 설정
	SpawnerSettings->OutAttributeName = FName("TreeInstance");
	SpawnerSettings->bApplyMeshBoundsToPoints = true;

	UPCGNode* SpawnerNode = PCGGraph->AddNode(SpawnerSettings);

	// 노드 연결: Forest Generator -> Mesh Spawner
	ForestNode->GetOutputPin(PCGPinConstants::DefaultOutputLabel)->AddEdgeTo(
		SpawnerNode->GetInputPin(PCGPinConstants::DefaultInputLabel)
	);

	// 출력 노드 설정
	SpawnerNode->GetOutputPin(PCGPinConstants::DefaultOutputLabel)->AddEdgeTo(
		PCGGraph->GetOutputNode()->GetInputPin(PCGPinConstants::DefaultInputLabel)
	);

	// PCG 생성 실행
	PCGComponent->Generate();

	UE_LOG(LogTemp, Log, TEXT("PCG Forest generated with parameters: Density=%s, MinDist=%.1f"),
		*Parameters.Density, Parameters.MinDistance);

	// PCG 그래프를 에셋으로 저장 (에디터에서만, 설정이 활성화된 경우)
#if WITH_EDITOR
	if (bSavePCGGraphAsAsset && !PCGGraphAsset)
	{
		SavePCGGraphAsAsset();
	}
#endif
}

void AForestPCGManager::LoadDefaultTreeMesh()
{
	// 엔진 기본 큐브 메시 사용 (하위 호환성)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		TreeMesh = CubeMeshFinder.Object;
		UE_LOG(LogTemp, Log, TEXT("Default tree mesh (Cube) loaded"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load default cube mesh"));
	}
}

void AForestPCGManager::InitializeDefaultTreeMeshes()
{
	// nlp_handler.py의 tree_types와 동일한 타입들
	// 기본값으로 엔진 큐브 메시 사용 (사용자가 에디터에서 변경 가능)

	TSoftObjectPtr<UStaticMesh> DefaultCubeMesh(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube")));

	// TreeMeshes가 비어있을 때만 초기화 (사용자 설정 보존)
	if (TreeMeshes.Num() == 0)
	{
		TreeMeshes.Add(TEXT("pine"), DefaultCubeMesh);          // 소나무
		TreeMeshes.Add(TEXT("oak"), DefaultCubeMesh);           // 참나무
		TreeMeshes.Add(TEXT("birch"), DefaultCubeMesh);         // 자작나무
		TreeMeshes.Add(TEXT("maple"), DefaultCubeMesh);         // 단풍나무
		TreeMeshes.Add(TEXT("generic_tree"), DefaultCubeMesh);  // 일반 나무

		UE_LOG(LogTemp, Log, TEXT("Initialized default tree meshes (all set to Cube). Please configure in Details panel."));
	}
}

UStaticMesh* AForestPCGManager::GetTreeMeshForType(const FString& TreeType)
{
	// TreeMeshes 맵에서 해당 타입의 메시 찾기
	if (TreeMeshes.Contains(TreeType))
	{
		TSoftObjectPtr<UStaticMesh> MeshPtr = TreeMeshes[TreeType];

		// Soft Object Pointer를 실제 객체로 로드
		UStaticMesh* LoadedMesh = MeshPtr.LoadSynchronous();
		if (LoadedMesh)
		{
			return LoadedMesh;
		}
	}

	// 찾지 못하면 generic_tree 시도
	if (TreeType != TEXT("generic_tree") && TreeMeshes.Contains(TEXT("generic_tree")))
	{
		TSoftObjectPtr<UStaticMesh> GenericMeshPtr = TreeMeshes[TEXT("generic_tree")];
		UStaticMesh* LoadedMesh = GenericMeshPtr.LoadSynchronous();
		if (LoadedMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Tree type '%s' not found, using generic_tree"), *TreeType);
			return LoadedMesh;
		}
	}

	// 그래도 없으면 레거시 TreeMesh 반환
	if (TreeMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Using legacy TreeMesh for type '%s'"), *TreeType);
		return TreeMesh;
	}

	UE_LOG(LogTemp, Error, TEXT("No mesh found for tree type '%s'"), *TreeType);
	return nullptr;
}

bool AForestPCGManager::SavePCGGraphAsAsset()
{
#if WITH_EDITOR
	if (!PCGComponent || !PCGComponent->GetGraph())
	{
		UE_LOG(LogTemp, Error, TEXT("No PCG Graph to save"));
		return false;
	}

	UPCGGraph* GraphToSave = PCGComponent->GetGraph();

	// 에셋 경로 생성
	FString AssetName = FString::Printf(TEXT("PCG_ForestGraph_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	FString PackagePath = GraphAssetPath + AssetName;

	// 패키지 생성
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackagePath);
		return false;
	}

	// 그래프를 패키지로 복사
	UPCGGraph* NewGraph = DuplicateObject<UPCGGraph>(GraphToSave, Package, *AssetName);
	if (!NewGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to duplicate PCG Graph"));
		return false;
	}

	// 에셋으로 마크
	NewGraph->SetFlags(RF_Public | RF_Standalone);

	// 패키지를 더티로 마크
	Package->MarkPackageDirty();

	// 에셋 레지스트리에 알림
	FAssetRegistryModule::AssetCreated(NewGraph);

	// 저장 (UE5.7 API)
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	bool bSaved = UPackage::SavePackage(Package, NewGraph, *PackageFileName, SaveArgs);

	if (bSaved)
	{
		// 새로 저장된 그래프를 PCGGraphAsset에 할당
		PCGGraphAsset = NewGraph;
		PCGComponent->SetGraph(NewGraph);

		UE_LOG(LogTemp, Log, TEXT("PCG Graph saved as asset: %s"), *PackagePath);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save PCG Graph asset"));
		return false;
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("SavePCGGraphAsAsset is only available in editor"));
	return false;
#endif
}

void AForestPCGManager::InitializeMCPClient()
{
	// 자동 생성 플래그가 꺼져 있으면 스킵
	if (!bAutoCreateMCPClient)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// MCP 클라이언트가 없으면 레벨에서 찾거나 생성
	if (!MCPClient)
	{
		// 레벨에서 기존 MCP Client 찾기
		for (TActorIterator<AMCPClient> It(World); It; ++It)
		{
			MCPClient = *It;
			UE_LOG(LogTemp, Warning, TEXT("ForestPCGManager: Found existing MCP Client in level"));
			break;
		}

		// 없으면 새로 생성
		if (!MCPClient)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(TEXT("AutoMCPClient"));
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			MCPClient = World->SpawnActor<AMCPClient>(AMCPClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			if (MCPClient)
			{
				MCPClient->bDebugMode = true;
				UE_LOG(LogTemp, Warning, TEXT("ForestPCGManager: Auto-created MCP Client (Debug Mode: ON)"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ForestPCGManager: Failed to create MCP Client"));
				return;
			}
		}
	}

	// 델리게이트 바인딩 (중복 방지를 위해 기존 바인딩 제거 후 추가)
	if (MCPClient)
	{
		// 기존 바인딩 제거
		MCPClient->OnForestGenerated.RemoveAll(this);

		// 새로 바인딩
		MCPClient->OnForestGenerated.AddDynamic(this, &AForestPCGManager::OnForestParametersReceived);
		UE_LOG(LogTemp, Warning, TEXT("ForestPCGManager: MCP Client bound to Forest Manager"));
		UE_LOG(LogTemp, Warning, TEXT("=== NLPPCG System Ready ==="));
		UE_LOG(LogTemp, Warning, TEXT("You can now generate forests using natural language commands!"));
		UE_LOG(LogTemp, Warning, TEXT("Example: '밀집된 소나무 숲'"));
	}
}
