// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGTerrainAdapter.h"
#include "PCGContext.h"
#include "PCGComponent.h"
#include "Data/PCGPointData.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UPCGTerrainAdapterSettings::UPCGTerrainAdapterSettings()
{
}

TArray<FPCGPinProperties> UPCGTerrainAdapterSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultInputLabel, EPCGDataType::Point);
	return PinProperties;
}

TArray<FPCGPinProperties> UPCGTerrainAdapterSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return PinProperties;
}

FPCGElementPtr UPCGTerrainAdapterSettings::CreateElement() const
{
	return MakeShared<FPCGTerrainAdapterElement>();
}

bool FPCGTerrainAdapterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGTerrainAdapterElement::Execute);

	const UPCGTerrainAdapterSettings* Settings = Context->GetInputSettings<UPCGTerrainAdapterSettings>();
	check(Settings);

	// 입력 데이터 가져오기
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	if (Inputs.Num() == 0)
	{
		return true;
	}

	UWorld* World = Context->SourceComponent.IsValid() ? Context->SourceComponent->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("World not found"));
		return true;
	}

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGPointData* InputPointData = Cast<UPCGPointData>(Input.Data);
		if (!InputPointData)
		{
			continue;
		}

		UPCGPointData* OutputPointData = NewObject<UPCGPointData>();
		TArray<FPCGPoint>& OutputPoints = OutputPointData->GetMutablePoints();

		const TArray<FPCGPoint>& InputPoints = InputPointData->GetPoints();

		for (const FPCGPoint& InputPoint : InputPoints)
		{
			FVector Location = InputPoint.Transform.GetLocation();
			FVector TerrainNormal;

			// 지형에 투영
			if (!ProjectToTerrain(World, Location, TerrainNormal, Settings->RaycastDistance))
			{
				// 지형을 찾지 못하면 스킵
				continue;
			}

			// 경사 계산
			float Slope = CalculateSlope(TerrainNormal);

			// 경사 필터
			if (Slope < Settings->TerrainFilter.MinSlope || Slope > Settings->TerrainFilter.MaxSlope)
			{
				continue;
			}

			// 고도 필터
			if (Location.Z < Settings->TerrainFilter.MinAltitude || Location.Z > Settings->TerrainFilter.MaxAltitude)
			{
				continue;
			}

			// 새 포인트 생성
			FPCGPoint& OutputPoint = OutputPoints.Add_GetRef(InputPoint);
			OutputPoint.Transform.SetLocation(Location);

			// 지형에 정렬
			if (Settings->TerrainFilter.bAlignToTerrain)
			{
				FQuat CurrentRotation = InputPoint.Transform.GetRotation();
				FQuat TerrainRotation = FRotationMatrix::MakeFromZ(TerrainNormal).ToQuat();

				// 정렬 강도에 따라 보간
				FQuat FinalRotation = FQuat::Slerp(
					CurrentRotation,
					TerrainRotation,
					Settings->TerrainFilter.AlignmentStrength
				);

				OutputPoint.Transform.SetRotation(FinalRotation);
			}

			// 경사에 따른 밀도 조정
			if (Settings->TerrainFilter.bAdjustDensityBySlope)
			{
				// 경사가 클수록 밀도 감소 (0-45도 범위)
				float DensityMultiplier = 1.0f - (Slope / 45.0f) * 0.5f;
				OutputPoint.Density *= DensityMultiplier;
			}

			// 경사에 따른 스케일 조정
			if (Settings->TerrainFilter.bAdjustScaleBySlope)
			{
				// 경사가 클수록 약간 작게
				float ScaleMultiplier = 1.0f - (Slope / 45.0f) * 0.2f;
				FVector CurrentScale = OutputPoint.Transform.GetScale3D();
				OutputPoint.Transform.SetScale3D(CurrentScale * ScaleMultiplier);
			}

			// 디버그 표시
			if (Settings->bShowDebug)
			{
				DrawDebugLine(World, Location, Location + TerrainNormal * 100.0f, FColor::Green, false, 5.0f);
				DrawDebugSphere(World, Location, 20.0f, 8, FColor::Blue, false, 5.0f);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Terrain adapter: %d -> %d points"), InputPoints.Num(), OutputPoints.Num());

		FPCGTaggedData& Output = Outputs.Emplace_GetRef();
		Output.Data = OutputPointData;
		Output.Tags = Input.Tags;
	}

	return true;
}

FVector FPCGTerrainAdapterElement::CalculateTerrainNormal(UWorld* World, const FVector& Location, float SampleDistance) const
{
	// 4방향 샘플링으로 법선 근사
	FVector Points[4];
	FVector Normal;

	FVector Directions[4] = {
		FVector(SampleDistance, 0, 0),
		FVector(-SampleDistance, 0, 0),
		FVector(0, SampleDistance, 0),
		FVector(0, -SampleDistance, 0)
	};

	for (int32 i = 0; i < 4; i++)
	{
		FVector SampleLocation = Location + Directions[i];
		FVector DummyNormal;
		if (!ProjectToTerrain(World, SampleLocation, DummyNormal, 10000.0f))
		{
			Points[i] = Location;
		}
		else
		{
			Points[i] = SampleLocation;
		}
	}

	// 크로스 프로덕트로 법선 계산
	FVector V1 = Points[0] - Points[1];
	FVector V2 = Points[2] - Points[3];
	Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();

	if (Normal.Z < 0)
	{
		Normal = -Normal;
	}

	return Normal;
}

float FPCGTerrainAdapterElement::CalculateSlope(const FVector& Normal) const
{
	// Z 축과의 각도 계산 (도)
	float DotProduct = FVector::DotProduct(Normal, FVector::UpVector);
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

bool FPCGTerrainAdapterElement::ProjectToTerrain(UWorld* World, FVector& InOutLocation, FVector& OutNormal, float RaycastDistance) const
{
	// 위에서 아래로 레이캐스트
	FVector Start = InOutLocation + FVector(0, 0, RaycastDistance * 0.5f);
	FVector End = InOutLocation - FVector(0, 0, RaycastDistance * 0.5f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		InOutLocation = HitResult.ImpactPoint;
		OutNormal = HitResult.ImpactNormal;
		return true;
	}

	OutNormal = FVector::UpVector;
	return false;
}
