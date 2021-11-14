// Copyright 2021 LexLiu. All Rights Reserved.

#include "LGUIWorldParticleSystemComponent.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRenderer.h"
#include "Core/LGUIMesh/LGUIMeshComponent.h"
#include "Core/LGUIIndexBuffer.h"

#define MESH_CREATE_OR_UPDATE 0
//PRAGMA_DISABLE_OPTIMIZATION

ALGUIParticleSystemActor::ALGUIParticleSystemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}
ULGUIWorldParticleSystemComponent* ALGUIParticleSystemActor::Emit(UNiagaraSystem* NiagaraSystemTemplate, bool AutoActivate)
{
	// Find old component
	TArray<ULGUIWorldParticleSystemComponent*> OldComponents;
	GetComponents<ULGUIWorldParticleSystemComponent>(OldComponents);

	// And destroy it
	for (ULGUIWorldParticleSystemComponent* Component : OldComponents)
		Component->DestroyComponent();


	auto ParticleComponent = NewObject<ULGUIWorldParticleSystemComponent>(this);

	ParticleComponent->SetAutoActivate(AutoActivate);
	ParticleComponent->SetAsset(NiagaraSystemTemplate);
	ParticleComponent->SetHiddenInGame(true);
	ParticleComponent->RegisterComponent();
	ParticleComponent->SetAutoDestroy(false);
	//ParticleComponent->SetForceSolo(true);
	this->Niagara = ParticleComponent;
	this->RootComponent = ParticleComponent;
	return ParticleComponent;
}

TArray<FLGUINiagaraRendererEntry> ULGUIWorldParticleSystemComponent::GetRenderEntries()
{
	TArray<FLGUINiagaraRendererEntry> Renderers;

	for (TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst : GetSystemInstance()->GetEmitters())
	{
		if (UNiagaraEmitter* Emitter = EmitterInst->GetCachedEmitter())
		{
			if (Emitter->SimTarget == ENiagaraSimTarget::CPUSim)
			{
				auto& Properties = Emitter->GetRenderers();

				for (UNiagaraRendererProperties* Property : Properties)
				{
					if (Property->GetIsEnabled() && Property->IsSimTargetSupported(Emitter->SimTarget))
					{
						if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Property))
						{
							FLGUINiagaraRendererEntry NewEntry(Property, EmitterInst, Emitter, SpriteRenderer->Material);
							Renderers.Add(NewEntry);
						}
						else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Property))
						{
							FLGUINiagaraRendererEntry NewEntry(Property, EmitterInst, Emitter, RibbonRenderer->Material);
							Renderers.Add(NewEntry);
						}
					}
				}
			}
		}
	}

	Algo::Sort(Renderers, [](FLGUINiagaraRendererEntry& FirstElement, FLGUINiagaraRendererEntry& SecondElement) {return FirstElement.RendererProperties->SortOrderHint < SecondElement.RendererProperties->SortOrderHint; });

	return Renderers;
}

void ULGUIWorldParticleSystemComponent::SetTransformationForUIRendering(FVector2D Location, FVector2D Scale, float Angle)
{
	const FVector NewLocation(Location.X, 0, Location.Y);
	const FVector NewScale(Scale.X, 1, Scale.Y);
	const FRotator NewRotation(0.f, 0.f, FMath::RadiansToDegrees(Angle));

	SetRelativeTransform(FTransform(NewRotation, NewLocation, NewScale));
}

void ULGUIWorldParticleSystemComponent::RenderUI(TWeakPtr<FLGUIMeshSection> UIMeshSection, FLGUINiagaraRendererEntry RendererEntry, float ScaleFactor, FVector2D LocationOffset, float Alpha01)
{
	if (!GetSystemInstance())
		return;

	if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(RendererEntry.RendererProperties))
	{
		AddSpriteRendererData(UIMeshSection, RendererEntry.Emitter->GetMaxParticleCountEstimate(), RendererEntry.EmitterInstance, SpriteRenderer, ScaleFactor, LocationOffset, Alpha01);
	}
	else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(RendererEntry.RendererProperties))
	{
		AddRibbonRendererData(UIMeshSection, RendererEntry.Emitter->GetMaxParticleCountEstimate(), RendererEntry.EmitterInstance, RibbonRenderer, ScaleFactor, LocationOffset, Alpha01);
	}
}

FORCEINLINE FVector2D FastRotate(const FVector2D Vector, float Sin, float Cos)
{
	return FVector2D(Cos * Vector.X - Sin * Vector.Y,
		Sin * Vector.X + Cos * Vector.Y);
}

void ULGUIWorldParticleSystemComponent::AddSpriteRendererData(TWeakPtr<FLGUIMeshSection> UIMeshSection, int32 MaxParticleCount
	, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
	, UNiagaraSpriteRendererProperties* SpriteRenderer
	, float ScaleFactor, FVector2D LocationOffset, float Alpha01
)
{
	FVector ComponentLocation = this->GetRelativeLocation();
	FVector ComponentScale = this->GetRelativeScale3D();
	FRotator ComponentRotation = this->GetRelativeRotation();
	float ComponentPitchRadians = FMath::DegreesToRadians(ComponentRotation.Pitch);

	FNiagaraDataSet& DataSet = EmitterInst->GetData();
	FNiagaraDataBuffer& ParticleData = DataSet.GetCurrentDataChecked();
	const int32 ParticleCount = ParticleData.GetNumInstances();

	if (ParticleCount < 1)
		return;

	bool LocalSpace = EmitterInst->GetCachedEmitter()->bLocalSpace;

	//const float FakeDepthScaler = 1 / WidgetProperties->FakeDepthScaleDistance;

	FVector2D SubImageSize = SpriteRenderer->SubImageSize;
	FVector2D SubImageDelta = FVector2D::UnitVector / SubImageSize;

	const auto PositionData = FNiagaraDataSetAccessor<FVector>::CreateReader(DataSet, SpriteRenderer->PositionBinding.GetDataSetBindableVariable().GetName());
	const auto ColorData = FNiagaraDataSetAccessor<FLinearColor>::CreateReader(DataSet, SpriteRenderer->ColorBinding.GetDataSetBindableVariable().GetName());
	const auto VelocityData = FNiagaraDataSetAccessor<FVector>::CreateReader(DataSet, SpriteRenderer->VelocityBinding.GetDataSetBindableVariable().GetName());
	const auto SizeData = FNiagaraDataSetAccessor<FVector2D>::CreateReader(DataSet, SpriteRenderer->SpriteSizeBinding.GetDataSetBindableVariable().GetName());
	const auto RotationData = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, SpriteRenderer->SpriteRotationBinding.GetDataSetBindableVariable().GetName());
	const auto SubImageData = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, SpriteRenderer->SubImageIndexBinding.GetDataSetBindableVariable().GetName());
	const auto DynamicMaterialData = FNiagaraDataSetAccessor<FVector4>::CreateReader(DataSet, SpriteRenderer->DynamicMaterialBinding.GetDataSetBindableVariable().GetName());

	auto GetParticlePosition2D = [&PositionData](int32 Index)
	{
		const FVector Position3D = PositionData.GetSafe(Index, FVector::ZeroVector);
		return FVector2D(Position3D.X, Position3D.Z);
	};

	auto GetParticleDepth = [&PositionData](int32 Index)
	{
		return PositionData.GetSafe(Index, FVector::ZeroVector).Y;
	};

	auto GetParticleColor = [&ColorData](int32 Index)
	{
		return ColorData.GetSafe(Index, FLinearColor::White);
	};

	auto GetParticleVelocity2D = [&VelocityData](int32 Index)
	{
		const FVector Velocity3D = VelocityData.GetSafe(Index, FVector::ZeroVector);
		return FVector2D(Velocity3D.X, -Velocity3D.Z);
	};

	auto GetParticleSize = [&SizeData](int32 Index)
	{
		return SizeData.GetSafe(Index, FVector2D::ZeroVector);
	};

	auto GetParticleRotation = [&RotationData](int32 Index)
	{
		return RotationData.GetSafe(Index, 0.f);
	};

	auto GetParticleSubImage = [&SubImageData](int32 Index)
	{
		return SubImageData.GetSafe(Index, 0.f);
	};

	auto GetDynamicMaterialData = [&DynamicMaterialData](int32 Index)
	{
		return DynamicMaterialData.GetSafe(Index, FVector4(0.f, 0.f, 0.f, 0.f));
	};
	
	int VertexCount = ParticleCount * 4;
	int IndexCount = ParticleCount * 6;
	auto& VertexData = UIMeshSection.Pin()->vertices;
	auto& IndexData = UIMeshSection.Pin()->triangles;
#if MESH_CREATE_OR_UPDATE
	if (VertexData.Num() != VertexCount)
	{
		VertexData.SetNumZeroed(VertexCount);
	}
	if (IndexData.Num() != IndexCount)
	{
		IndexData.SetNumZeroed(IndexCount);
	}
#else
	const int ParticleCountIncrease = 30;//每增加n个粒子才创建新的RenderResource，这样可以减少消耗
	const int VertexCountIncreaseSize = ParticleCountIncrease * 4;
	const int IndexCountIncreaseSize = ParticleCountIncrease * 6;
	if (VertexData.Num() < VertexCount)
	{
		int ExtraCountNeeded = VertexCount - VertexData.Num();
		ExtraCountNeeded = ((ExtraCountNeeded / VertexCountIncreaseSize) + 1) * VertexCountIncreaseSize;
		VertexData.AddZeroed(ExtraCountNeeded);
	}
	if (IndexData.Num() < IndexCount)
	{
		int ExtraCountNeeded = IndexCount - IndexData.Num();
		ExtraCountNeeded = ((ExtraCountNeeded / IndexCountIncreaseSize) + 1) * IndexCountIncreaseSize;
		IndexData.AddZeroed(ExtraCountNeeded);
	}
	else if (IndexData.Num() > IndexCount)//set not required triangle index to zero
	{
		FMemory::Memzero(((uint8*)IndexData.GetData()) + IndexCount * sizeof(FLGUIIndexType), (IndexData.Num() - IndexCount) * sizeof(FLGUIIndexType));
	}
#endif

	for (int ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
	{

		FVector2D ParticlePosition = GetParticlePosition2D(ParticleIndex) * ScaleFactor;
		FVector2D ParticleSize = GetParticleSize(ParticleIndex) * ScaleFactor;

		if (LocalSpace)
		{
			ParticlePosition *= FVector2D(ComponentScale.X, ComponentScale.Z);
			ParticlePosition = ParticlePosition.GetRotated(-ComponentRotation.Pitch);
			ParticlePosition += LocationOffset;
			ParticlePosition += FVector2D(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;

			ParticleSize *= FVector2D(ComponentScale.X, ComponentScale.Z);
		}
		else
		{
			ParticlePosition += LocationOffset;
		}


		//if (WidgetProperties->FakeDepthScale)
		//{
		//	const float ParticleDepth = (-GetParticleDepth(ParticleIndex) + WidgetProperties->FakeDepthScaleDistance) * FakeDepthScaler;
		//	ParticleSize *= ParticleDepth;
		//}


		const FVector2D ParticleHalfSize = ParticleSize * 0.5;


		FColor ParticleColor = GetParticleColor(ParticleIndex).ToFColor(false);
		ParticleColor.A = ParticleColor.A * Alpha01;


		float ParticleRotationSin = 0, ParticleRotationCos = 0;

		if (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
		{
			const FVector2D ParticleVelocity = GetParticleVelocity2D(ParticleIndex);

			ParticleRotationCos = FVector2D::DotProduct(ParticleVelocity.GetSafeNormal(), FVector2D(0.f, 1.f));
			const float SinSign = FMath::Sign(FVector2D::DotProduct(ParticleVelocity, FVector2D(1.f, 0.f)));

			if (LocalSpace)
			{
				const float ParticleRotation = FMath::Acos(ParticleRotationCos * SinSign) - ComponentPitchRadians;
				FMath::SinCos(&ParticleRotationSin, &ParticleRotationCos, ParticleRotation);
			}
			else
			{
				ParticleRotationSin = FMath::Sqrt(1 - ParticleRotationCos * ParticleRotationCos) * SinSign;
			}
		}
		else
		{
			float ParticleRotation = GetParticleRotation(ParticleIndex);

			if (LocalSpace)
				ParticleRotation -= ComponentRotation.Pitch;

			FMath::SinCos(&ParticleRotationSin, &ParticleRotationCos, FMath::DegreesToRadians(ParticleRotation));
		}
		if (!FMath::IsFinite(ParticleRotationSin))ParticleRotationSin = 0.0f;
		if (!FMath::IsFinite(ParticleRotationCos))ParticleRotationCos = 0.0f;

		FVector2D TextureCoordinates[4];

		if (SubImageSize != FVector2D(1.f, 1.f))
		{
			const float ParticleSubImage = GetParticleSubImage(ParticleIndex);
			const int Row = (int)FMath::Floor(ParticleSubImage / SubImageSize.X) % (int)SubImageSize.Y;
			const int Column = (int)(ParticleSubImage) % (int)(SubImageSize.X);

			const float LeftUV = SubImageDelta.X * Column;
			const float Right = SubImageDelta.X * (Column + 1);
			const float TopUV = SubImageDelta.Y * Row;
			const float BottomUV = SubImageDelta.Y * (Row + 1);

			TextureCoordinates[0] = FVector2D(LeftUV, TopUV);
			TextureCoordinates[1] = FVector2D(Right, TopUV);
			TextureCoordinates[2] = FVector2D(LeftUV, BottomUV);
			TextureCoordinates[3] = FVector2D(Right, BottomUV);
		}
		else
		{
			TextureCoordinates[0] = FVector2D(0.f, 0.f);
			TextureCoordinates[1] = FVector2D(1.f, 0.f);
			TextureCoordinates[2] = FVector2D(0.f, 1.f);
			TextureCoordinates[3] = FVector2D(1.f, 1.f);
		}


		const FVector4 MaterialData = GetDynamicMaterialData(ParticleIndex);

		FVector2D PositionArray[4];
		PositionArray[0] = FastRotate(FVector2D(-ParticleHalfSize.X, -ParticleHalfSize.Y), ParticleRotationSin, ParticleRotationCos);
		PositionArray[1] = FastRotate(FVector2D(ParticleHalfSize.X, -ParticleHalfSize.Y), ParticleRotationSin, ParticleRotationCos);
		PositionArray[2] = -PositionArray[1];
		PositionArray[3] = -PositionArray[0];

		const int VertexIndex = ParticleIndex * 4;
		const int indexIndex = ParticleIndex * 6;


		for (int i = 0; i < 4; ++i)
		{
			VertexData[VertexIndex + i].Position = FVector(PositionArray[i] + ParticlePosition, 0);
			VertexData[VertexIndex + i].Color = ParticleColor;
			VertexData[VertexIndex + i].TextureCoordinate[0] = TextureCoordinates[i];
			VertexData[VertexIndex + i].TextureCoordinate[1].X = MaterialData.X;
			VertexData[VertexIndex + i].TextureCoordinate[1].Y = MaterialData.Y;
			VertexData[VertexIndex + i].TextureCoordinate[2].X = MaterialData.Z;
			VertexData[VertexIndex + i].TextureCoordinate[2].Y = MaterialData.W;
		}


		IndexData[indexIndex] = VertexIndex;
		IndexData[indexIndex + 1] = VertexIndex + 1;
		IndexData[indexIndex + 2] = VertexIndex + 2;

		IndexData[indexIndex + 3] = VertexIndex + 2;
		IndexData[indexIndex + 4] = VertexIndex + 1;
		IndexData[indexIndex + 5] = VertexIndex + 3;
	}
}

void ULGUIWorldParticleSystemComponent::AddRibbonRendererData(TWeakPtr<FLGUIMeshSection> UIMeshSection, int32 MaxParticleCount
	, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
	, UNiagaraRibbonRendererProperties* RibbonRenderer
	, float ScaleFactor, FVector2D LocationOffset, float Alpha01
)
{
	FVector ComponentLocation = GetRelativeLocation();
	FVector ComponentScale = GetRelativeScale3D();
	FRotator ComponentRotation = GetRelativeRotation();

	FNiagaraDataSet& DataSet = EmitterInst->GetData();
	FNiagaraDataBuffer& ParticleData = DataSet.GetCurrentDataChecked();
	const int32 ParticleCount = ParticleData.GetNumInstances();

	if (ParticleCount < 2)
		return;


	const auto SortKeyReader = RibbonRenderer->SortKeyDataSetAccessor.GetReader(DataSet);

	const auto PositionData = RibbonRenderer->PositionDataSetAccessor.GetReader(DataSet);
	const auto ColorData = FNiagaraDataSetAccessor<FLinearColor>::CreateReader(DataSet, RibbonRenderer->ColorBinding.GetDataSetBindableVariable().GetName());
	const auto RibbonWidthData = RibbonRenderer->SizeDataSetAccessor.GetReader(DataSet);

	const auto RibbonFullIDData = RibbonRenderer->RibbonFullIDDataSetAccessor.GetReader(DataSet);

	auto GetParticlePosition2D = [&PositionData](int32 Index)
	{
		const FVector Position3D = PositionData.GetSafe(Index, FVector::ZeroVector);
		return FVector2D(Position3D.X, Position3D.Z);
	};

	auto GetParticleColor = [&ColorData](int32 Index)
	{
		return ColorData.GetSafe(Index, FLinearColor::White);
	};

	auto GetParticleWidth = [&RibbonWidthData](int32 Index)
	{
		return RibbonWidthData.GetSafe(Index, 0.f);
	};

	auto AngleLargerThanPi = [](const FVector2D& A, const FVector2D& B)
	{
		float temp = A.X * B.Y - B.X * A.Y;
		return temp < 0;
	};

	auto GenerateLinePoint = [AngleLargerThanPi](const FVector2D& InCurrentPoint, const FVector2D& InPrevPoint, const FVector2D& InNextPoint
		, float InLineLeftWidth, float InLineRightWidth
		, FVector2D& OutPosA, FVector2D& OutPosB
		)
	{
		FVector2D normalizedV1 = (InPrevPoint - InCurrentPoint).GetSafeNormal();
		FVector2D normalizedV2 = (InNextPoint - InCurrentPoint).GetSafeNormal();
		if (normalizedV1 == -normalizedV2)
		{
			auto itemNormal = FVector2D(normalizedV2.Y, -normalizedV2.X);
			OutPosA = InCurrentPoint + InLineLeftWidth * itemNormal;
			OutPosB = InCurrentPoint - InLineRightWidth * itemNormal;
		}
		else
		{
			auto itemNormal = normalizedV1 + normalizedV2;
			itemNormal.Normalize();
			if (itemNormal.X == 0 && itemNormal.Y == 0)//wrong normal
			{
				itemNormal = FVector2D(normalizedV2.Y, -normalizedV2.X);
			}
			float prevDotN = FVector2D::DotProduct(normalizedV1, itemNormal);
			float angle = FMath::Acos(prevDotN);
			float sin = FMath::Sin(angle);
			itemNormal = AngleLargerThanPi(normalizedV1, normalizedV2) ? -itemNormal : itemNormal;
			OutPosA = InCurrentPoint + InLineLeftWidth / sin * itemNormal;
			OutPosB = InCurrentPoint - InLineRightWidth / sin * itemNormal;
		}
	};

	const bool LocalSpace = EmitterInst->GetCachedEmitter()->bLocalSpace;
	const bool FullIDs = RibbonFullIDData.IsValid();
	const bool MultiRibbons = FullIDs;

	auto& VertexData = UIMeshSection.Pin()->vertices;
	auto& IndexData = UIMeshSection.Pin()->triangles;

	auto AddRibbonVerts = [&](TArray<int32>& RibbonIndices, int32 CurrentVertexIndex, int32 CurrentIndexIndex, int32& OutVertexCount, int32& OutIndexCount)
	{
		const int32 numParticlesInRibbon = RibbonIndices.Num();
#if 1
		if (numParticlesInRibbon < 3)
			return;

		int VertexCount = (numParticlesInRibbon - 1) * 2;
		int IndexCount = (numParticlesInRibbon - 2) * 6;
		if (VertexData.Num() < CurrentVertexIndex + VertexCount)
		{
			VertexData.AddZeroed(VertexCount);
		}
		if (IndexData.Num() < CurrentIndexIndex + IndexCount)
		{
			IndexData.AddZeroed(IndexCount);
		}
		OutVertexCount = VertexCount;
		OutIndexCount = IndexCount;

		const int32 StartDataIndex = RibbonIndices[0];

		float TotalDistance = 0.0f;

		FVector2D LastPosition = GetParticlePosition2D(StartDataIndex);
		FVector2D CurrentPosition = FVector2D::ZeroVector;
		float CurrentWidth = 0.f;
		FVector2D LastToCurrentVector = FVector2D::ZeroVector;
		float LastToCurrentSize = 0.f;
		float LastU0 = 0.f;
		float LastU1 = 0.f;

		FVector2D LastParticleUIPosition = LastPosition * ScaleFactor;

		if (LocalSpace)
		{
			LastParticleUIPosition *= FVector2D(ComponentScale.X, ComponentScale.Z);
			LastParticleUIPosition = LastParticleUIPosition.GetRotated(-ComponentRotation.Pitch);
			LastParticleUIPosition += LocationOffset;

			LastParticleUIPosition += FVector2D(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;
		}
		else
		{
			LastParticleUIPosition += LocationOffset;
		}

		int32 CurrentIndex = 1;
		int32 CurrentDataIndex = RibbonIndices[CurrentIndex];

		CurrentPosition = GetParticlePosition2D(CurrentDataIndex);
		LastToCurrentVector = CurrentPosition - LastPosition;
		LastToCurrentSize = LastToCurrentVector.Size();

		// Normalize LastToCurrVec
		LastToCurrentVector *= 1.f / LastToCurrentSize;


		FColor InitialColor = GetParticleColor(StartDataIndex).ToFColor(false);
		InitialColor.A = InitialColor.A * Alpha01;
		const float InitialWidth = GetParticleWidth(StartDataIndex) * ScaleFactor;

		FVector2D InitialPositionArray[2];
		InitialPositionArray[0] = LastToCurrentVector.GetRotated(90.f) * InitialWidth * 0.5f;
		InitialPositionArray[1] = -InitialPositionArray[0];

		for (int i = 0; i < 2; ++i)
		{
			VertexData[CurrentVertexIndex + i].Position = FVector(InitialPositionArray[i] + LastParticleUIPosition, 0);
			VertexData[CurrentVertexIndex + i].Color = InitialColor;
			VertexData[CurrentVertexIndex + i].TextureCoordinate[0] = FVector2D(i, 0);
		}

		CurrentVertexIndex += 2;

		int32 NextIndex = CurrentIndex + 1;

		while (NextIndex < numParticlesInRibbon)
		{
			const int32 NextDataIndex = RibbonIndices[NextIndex];
			const FVector2D NextPosition = GetParticlePosition2D(NextDataIndex);
			FVector2D CurrentToNextVector = NextPosition - CurrentPosition;
			const float CurrentToNextSize = CurrentToNextVector.Size();
			CurrentWidth = GetParticleWidth(CurrentDataIndex) * ScaleFactor;
			FColor CurrentColor = GetParticleColor(CurrentDataIndex).ToFColor(false);
			CurrentColor.A = CurrentColor.A * Alpha01;

			// Normalize CurrToNextVec
			CurrentToNextVector *= 1.f / CurrentToNextSize;

			const FVector2D CurrentTangent = (LastToCurrentVector + CurrentToNextVector).GetSafeNormal();

			TotalDistance += LastToCurrentSize;

			FVector2D CurrentPositionArray[2];
			CurrentPositionArray[0] = CurrentTangent.GetRotated(90.f) * CurrentWidth * 0.5f;
			CurrentPositionArray[1] = -CurrentPositionArray[0];

			FVector2D CurrentParticleUIPosition = CurrentPosition * ScaleFactor;

			if (LocalSpace)
			{
				CurrentParticleUIPosition *= FVector2D(ComponentScale.X, ComponentScale.Z);
				CurrentParticleUIPosition = CurrentParticleUIPosition.GetRotated(-ComponentRotation.Pitch);
				CurrentParticleUIPosition += LocationOffset;
				CurrentParticleUIPosition += FVector2D(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;
			}
			else
			{
				CurrentParticleUIPosition += LocationOffset;
			}

			float CurrentU0 = 0.f;

			if (RibbonRenderer->UV0Settings.DistributionMode == ENiagaraRibbonUVDistributionMode::TiledOverRibbonLength)
			{
				CurrentU0 = LastU0 + LastToCurrentSize / RibbonRenderer->UV0Settings.TilingLength;
			}
			else
			{
				CurrentU0 = (float)CurrentIndex / (float)numParticlesInRibbon;
			}

			float CurrentU1 = 0.f;

			if (RibbonRenderer->UV1Settings.DistributionMode == ENiagaraRibbonUVDistributionMode::TiledOverRibbonLength)
			{
				CurrentU1 = LastU1 + LastToCurrentSize / RibbonRenderer->UV1Settings.TilingLength;
			}
			else
			{
				CurrentU1 = (float)CurrentIndex / (float)numParticlesInRibbon;
			}

			FVector2D TextureCoordinates0[2];
			TextureCoordinates0[0] = FVector2D(CurrentU0, 1.f);
			TextureCoordinates0[1] = FVector2D(CurrentU0, 0.f);

			FVector2D TextureCoordinates1[2];
			TextureCoordinates1[0] = FVector2D(CurrentU1, 1.f);
			TextureCoordinates1[1] = FVector2D(CurrentU1, 0.f);

			for (int i = 0; i < 2; ++i)
			{
				VertexData[CurrentVertexIndex + i].Position = FVector(CurrentPositionArray[i] + CurrentParticleUIPosition, 0);
				VertexData[CurrentVertexIndex + i].Color = CurrentColor;
				VertexData[CurrentVertexIndex + i].TextureCoordinate[0] = TextureCoordinates0[i];
				VertexData[CurrentVertexIndex + i].TextureCoordinate[1] = TextureCoordinates1[i];
			}

			IndexData[CurrentIndexIndex] = CurrentVertexIndex - 2;
			IndexData[CurrentIndexIndex + 1] = CurrentVertexIndex - 1;
			IndexData[CurrentIndexIndex + 2] = CurrentVertexIndex;

			IndexData[CurrentIndexIndex + 3] = CurrentVertexIndex;
			IndexData[CurrentIndexIndex + 4] = CurrentVertexIndex - 1;
			IndexData[CurrentIndexIndex + 5] = CurrentVertexIndex + 1;


			CurrentVertexIndex += 2;
			CurrentIndexIndex += 6;

			CurrentIndex = NextIndex;
			CurrentDataIndex = NextDataIndex;
			LastPosition = CurrentPosition;
			LastParticleUIPosition = CurrentParticleUIPosition;
			CurrentPosition = NextPosition;
			LastToCurrentVector = CurrentToNextVector;
			LastToCurrentSize = CurrentToNextSize;
			LastU0 = CurrentU0;

			++NextIndex;
		}
#else
		if (numParticlesInRibbon < 2)
			return;

		int VertexCount = numParticlesInRibbon * 2;
		int IndexCount = (numParticlesInRibbon - 1) * 6;
		if (VertexData.Num() < CurrentVertexIndex + VertexCount)
		{
			VertexData.AddZeroed(VertexCount);
		}
		if (IndexData.Num() < CurrentIndexIndex + IndexCount)
		{
			IndexData.AddZeroed(IndexCount);
		}
		OutVertexCount = VertexCount;
		OutIndexCount = IndexCount;

		int pointCount = numParticlesInRibbon;
		//triangles
		{
			int pointIndex = 0;
			int vertIndex = 0, triangleIndex = 0;
			for (int count = pointCount - 1; pointIndex < count; pointIndex++)
			{
				vertIndex = pointIndex * 2 + CurrentVertexIndex;
				triangleIndex = pointIndex * 6 + CurrentIndexIndex;
				IndexData[triangleIndex] = vertIndex;
				IndexData[triangleIndex + 1] = vertIndex + 2;
				IndexData[triangleIndex + 2] = vertIndex + 3;

				IndexData[triangleIndex + 3] = vertIndex;
				IndexData[triangleIndex + 4] = vertIndex + 3;
				IndexData[triangleIndex + 5] = vertIndex + 1;
			}
		}
		//start point
		{
			FVector2D pos0, pos1;
			FVector2D v0 = GetParticlePosition2D(RibbonIndices[0]);
			FVector2D v0to1 = GetParticlePosition2D(RibbonIndices[1]) - v0;
			FVector2D dir;
			FVector2D widthDir;

			float magnitude;
			v0to1.ToDirectionAndLength(dir, magnitude);
			widthDir = FVector2D(dir.Y, -dir.X);//rotate 90 degree

			FColor InitialColor = GetParticleColor(RibbonIndices[0]).ToFColor(false);
			InitialColor.A = InitialColor.A * Alpha01;
			const float lineWidth = GetParticleWidth(0) * 0.5f * ScaleFactor;
			pos0 = v0 + lineWidth * widthDir;
			pos1 = v0 - lineWidth * widthDir;

			auto& vert0 = VertexData[0 + CurrentVertexIndex];
			vert0.Position = FVector(pos0.X + LocationOffset.X, pos0.Y + LocationOffset.Y, 0);
			vert0.Color = InitialColor;
			vert0.TextureCoordinate[0] = FVector2D(0, 0);
			vert0.TextureCoordinate[1] = FVector2D(0, 0);

			auto& vert1 = VertexData[1 + CurrentVertexIndex];
			vert1.Position = FVector(pos1.X + LocationOffset.X, pos1.Y + LocationOffset.Y, 0);
			vert1.Color = InitialColor;
			vert1.TextureCoordinate[0] = FVector2D(1, 0);
			vert1.TextureCoordinate[1] = FVector2D(1, 0);
		}
		//middle points
		int i = 1;
		if (pointCount >= 3)
		{
			for (; i < pointCount - 1; i++)
			{
				FVector2D pos0, pos1;
				FVector2D originPoint = GetParticlePosition2D(RibbonIndices[i]);
				FVector2D originPrevPoint = GetParticlePosition2D(RibbonIndices[i - 1]);
				FVector2D originNextPoint = GetParticlePosition2D(RibbonIndices[i + 1]);
				FColor InitialColor = GetParticleColor(RibbonIndices[i]).ToFColor(false);
				InitialColor.A = InitialColor.A * Alpha01;
				const float lineWidth = GetParticleWidth(RibbonIndices[i]) * 0.5f * ScaleFactor;
				GenerateLinePoint(originPoint, originPrevPoint, originNextPoint, lineWidth, lineWidth, pos0, pos1);

				auto& vert0 = VertexData[i + i + CurrentVertexIndex];
				vert0.Position = FVector(pos0.X + LocationOffset.X, pos0.Y + LocationOffset.Y, 0);
				vert0.Color = InitialColor;
				vert0.TextureCoordinate[0] = FVector2D(0, 0);
				vert0.TextureCoordinate[1] = FVector2D(0, 0);

				auto& vert1 = VertexData[i + i + 1 + CurrentVertexIndex];
				vert1.Position = FVector(pos1.X + LocationOffset.X, pos1.Y + LocationOffset.Y, 0);
				vert1.Color = InitialColor;
				vert1.TextureCoordinate[0] = FVector2D(1, 0);
				vert1.TextureCoordinate[1] = FVector2D(1, 0);
			}
		}
		//end point
		{
			FVector2D vEnd2 = GetParticlePosition2D(RibbonIndices[pointCount - 2]);
			FVector2D vEnd1 = GetParticlePosition2D(RibbonIndices[pointCount - 1]);

			FVector2D v1to2 = vEnd1 - vEnd2;
			FVector2D dir;
			FVector2D widthDir;

			float magnitude;
			v1to2.ToDirectionAndLength(dir, magnitude);
			widthDir = FVector2D(dir.Y, -dir.X);//rotate 90 degree

			FColor InitialColor = GetParticleColor(RibbonIndices[i]).ToFColor(false);
			InitialColor.A = InitialColor.A * Alpha01;
			const float lineWidth = GetParticleWidth(RibbonIndices[i]) * 0.5f * ScaleFactor;
			auto pos0 = vEnd1 + lineWidth * widthDir;
			auto pos1 = vEnd1 - lineWidth * widthDir;

			auto& vert0 = VertexData[i + i + CurrentVertexIndex];
			vert0.Position = FVector(pos0.X + LocationOffset.X, pos0.Y + LocationOffset.Y, 0);
			vert0.Color = InitialColor;
			vert0.TextureCoordinate[0] = FVector2D(0, 0);
			vert0.TextureCoordinate[1] = FVector2D(0, 0);

			auto& vert1 = VertexData[i + i + 1 + CurrentVertexIndex];
			vert1.Position = FVector(pos1.X + LocationOffset.X, pos1.Y + LocationOffset.Y, 0);
			vert1.Color = InitialColor;
			vert1.TextureCoordinate[0] = FVector2D(1, 0);
			vert1.TextureCoordinate[1] = FVector2D(1, 0);
		}
#endif
	};

	if (!MultiRibbons)
	{
		TArray<int32> SortedIndices;
		for (int32 i = 0; i < ParticleCount; ++i)
		{
			SortedIndices.Add(i);
		}

		SortedIndices.Sort([&SortKeyReader](const int32& A, const int32& B) {	return (SortKeyReader[A] < SortKeyReader[B]); });
#if MESH_CREATE_OR_UPDATE
		VertexData.Reset();
		IndexData.Reset();
#endif
		int VertexCount = 0, IndexCount = 0;
		AddRibbonVerts(SortedIndices, 0, 0, VertexCount, IndexCount);
		if (IndexData.Num() > IndexCount)
		{
			FMemory::Memzero((void*)(IndexData.GetData() + IndexCount), (IndexData.Num() - IndexCount) * sizeof(FLGUIIndexType));
		}
	}
	else
	{
		if (FullIDs)
		{
			TMap<FNiagaraID, TArray<int32>> MultiRibbonSortedIndices;

			for (int32 i = 0; i < ParticleCount; ++i)
			{
				TArray<int32>& Indices = MultiRibbonSortedIndices.FindOrAdd(RibbonFullIDData[i]);
				Indices.Add(i);
			}

			// Sort the ribbons by ID so that the draw order stays consistent.
			MultiRibbonSortedIndices.KeySort(TLess<FNiagaraID>());
#if MESH_CREATE_OR_UPDATE
			VertexData.Reset();
			IndexData.Reset();
#endif
			int VertexCount = 0, IndexCount = 0;
			for (TPair<FNiagaraID, TArray<int32>>& Pair : MultiRibbonSortedIndices)
			{
				TArray<int32>& SortedIndices = Pair.Value;
				SortedIndices.Sort([&SortKeyReader](const int32& A, const int32& B) {	return (SortKeyReader[A] < SortKeyReader[B]); });
				int32 ThisVertexCount = 0, ThisIndexCount = 0;
				AddRibbonVerts(SortedIndices, VertexCount, IndexCount, ThisVertexCount, ThisIndexCount);
				VertexCount += ThisVertexCount;
				IndexCount += ThisIndexCount;
			}
#if !MESH_CREATE_OR_UPDATE
			if (IndexData.Num() > IndexCount)
			{
				FMemory::Memzero(((uint8*)IndexData.GetData()) + IndexCount * sizeof(FLGUIIndexType), (IndexData.Num() - IndexCount) * sizeof(FLGUIIndexType));
			}
#endif
		}
	}
}
//PRAGMA_ENABLE_OPTIMIZATION