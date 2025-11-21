// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGForestGenerator.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"

UPCGForestGeneratorSettings::UPCGForestGeneratorSettings()
{
	// 기본값 설정
}

TArray<FPCGPinProperties> UPCGForestGeneratorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	// 입력 핀 없음 (순수 생성자)
	return PinProperties;
}

TArray<FPCGPinProperties> UPCGForestGeneratorSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return PinProperties;
}

FPCGElementPtr UPCGForestGeneratorSettings::CreateElement() const
{
	return MakeShared<FPCGForestGeneratorElement>();
}

bool FPCGForestGeneratorElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGForestGeneratorElement::Execute);

	const UPCGForestGeneratorSettings* Settings = Context->GetInputSettings<UPCGForestGeneratorSettings>();
	check(Settings);

	const FPCGForestParameters& Params = Settings->ForestParameters;

	// 출력 데이터 생성
	UPCGPointData* PointData = NewObject<UPCGPointData>();
	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();

	// 랜덤 스트림 초기화
	FRandomStream RandomStream(Settings->Seed);

	// 경계 박스 설정
	FVector BoundsMin = -Settings->BoundsSize * 0.5f;
	FVector BoundsMax = Settings->BoundsSize * 0.5f;

	// Poisson Disk Sampling을 사용한 포인트 생성
	const float MinDist = Params.MinDistance;
	const int32 MaxAttempts = 30;
	const int32 GridSize = FMath::CeilToInt(Settings->BoundsSize.X / MinDist);

	// 그리드 기반 포인트 배치
	TArray<FVector> ActiveList;
	TArray<TArray<int32>> Grid;
	Grid.SetNum(GridSize * GridSize);

	// 첫 포인트 생성
	FVector FirstPoint = FVector(
		RandomStream.FRandRange(BoundsMin.X, BoundsMax.X),
		RandomStream.FRandRange(BoundsMin.Y, BoundsMax.Y),
		0.0f
	);
	ActiveList.Add(FirstPoint);

	auto GetGridIndex = [GridSize, MinDist, BoundsMin](const FVector& Point) -> int32
	{
		int32 X = FMath::FloorToInt((Point.X - BoundsMin.X) / MinDist);
		int32 Y = FMath::FloorToInt((Point.Y - BoundsMin.Y) / MinDist);
		X = FMath::Clamp(X, 0, GridSize - 1);
		Y = FMath::Clamp(Y, 0, GridSize - 1);
		return Y * GridSize + X;
	};

	auto IsValidPoint = [&](const FVector& Point) -> bool
	{
		if (Point.X < BoundsMin.X || Point.X > BoundsMax.X ||
			Point.Y < BoundsMin.Y || Point.Y > BoundsMax.Y)
		{
			return false;
		}

		int32 GridX = FMath::FloorToInt((Point.X - BoundsMin.X) / MinDist);
		int32 GridY = FMath::FloorToInt((Point.Y - BoundsMin.Y) / MinDist);

		// 주변 그리드 셀 확인
		for (int32 dx = -2; dx <= 2; ++dx)
		{
			for (int32 dy = -2; dy <= 2; ++dy)
			{
				int32 nx = GridX + dx;
				int32 ny = GridY + dy;

				if (nx >= 0 && nx < GridSize && ny >= 0 && ny < GridSize)
				{
					int32 GridIdx = ny * GridSize + nx;
					for (int32 PointIdx : Grid[GridIdx])
					{
						if (FVector::Distance(Point, Points[PointIdx].Transform.GetLocation()) < MinDist)
						{
							return false;
						}
					}
				}
			}
		}

		return true;
	};

	// 첫 포인트 추가
	FPCGPoint& FirstPCGPoint = Points.Add_GetRef(FPCGPoint());
	FirstPCGPoint.Transform = FTransform(FirstPoint);
	FirstPCGPoint.SetLocalBounds(FBox(FVector(-50.0f), FVector(50.0f)));
	FirstPCGPoint.Density = 1.0f;

	int32 FirstGridIdx = GetGridIndex(FirstPoint);
	Grid[FirstGridIdx].Add(0);

	// Poisson Disk Sampling
	while (ActiveList.Num() > 0 && Points.Num() < 10000) // 최대 10000개 제한
	{
		int32 RandomIndex = RandomStream.RandRange(0, ActiveList.Num() - 1);
		FVector Point = ActiveList[RandomIndex];

		bool bFoundValid = false;

		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			// MinDistance와 MaxDistance 사이의 거리에 새 포인트 생성
			float Angle = RandomStream.FRand() * 2.0f * PI;
			float Distance = RandomStream.FRandRange(Params.MinDistance, Params.MaxDistance);

			FVector NewPoint = Point + FVector(
				FMath::Cos(Angle) * Distance,
				FMath::Sin(Angle) * Distance,
				0.0f
			);

			if (IsValidPoint(NewPoint))
			{
				FPCGPoint& NewPCGPoint = Points.Add_GetRef(FPCGPoint());
				NewPCGPoint.Transform = FTransform(NewPoint);
				NewPCGPoint.SetLocalBounds(FBox(FVector(-50.0f), FVector(50.0f)));
				NewPCGPoint.Density = 1.0f;

				// 스케일 설정 (크기 파라미터 적용)
				float ScaleVariation = RandomStream.FRandRange(0.8f, 1.2f);
				float FinalScale = Params.ScaleMultiplier * ScaleVariation;
				NewPCGPoint.Transform.SetScale3D(FVector(FinalScale));

				// 랜덤 회전
				FRotator Rotation(0.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f);
				NewPCGPoint.Transform.SetRotation(Rotation.Quaternion());

				ActiveList.Add(NewPoint);

				int32 GridIdx = GetGridIndex(NewPoint);
				Grid[GridIdx].Add(Points.Num() - 1);

				bFoundValid = true;
				break;
			}
		}

		if (!bFoundValid)
		{
			ActiveList.RemoveAt(RandomIndex);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Generated %d tree points"), Points.Num());

	// 출력 데이터 추가
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	FPCGTaggedData& Output = Outputs.Emplace_GetRef();
	Output.Data = PointData;

	return true;
}
