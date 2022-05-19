// Copyright 2021-present LexLiu. All Rights Reserved.

#include "LGUIWorldParticleSystemComponent.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRenderer.h"
#include "Core/LGUIMesh/LGUIMeshComponent.h"
#include "Core/LGUIIndexBuffer.h"

//PRAGMA_DISABLE_OPTIMIZATION

ALGUIWorldParticleSystemActor::ALGUIWorldParticleSystemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}
ULGUIWorldParticleSystemComponent* ALGUIWorldParticleSystemActor::Emit(UNiagaraSystem* NiagaraSystemTemplate, bool AutoActivate)
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

void ULGUIWorldParticleSystemComponent::SetTransformationForUIRendering(MyVector2 Location, MyVector2 Scale, float Angle)
{
	const FVector NewLocation(Location.X, 0, Location.Y);
	const FVector NewScale(Scale.X, 1, Scale.Y);
	const FRotator NewRotation(0.f, 0.f, FMath::RadiansToDegrees(Angle));

	SetRelativeTransform(FTransform(NewRotation, NewLocation, NewScale));
}

void ULGUIWorldParticleSystemComponent::RenderUI(FLGUIMeshSection* UIMeshSection, FLGUINiagaraRendererEntry RendererEntry, float ScaleFactor, MyVector2 LocationOffset, float Alpha01, const int ParticleCountIncreaseAndDecrease)
{
	if (!GetSystemInstance())
		return;

	if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(RendererEntry.RendererProperties))
	{
		AddSpriteRendererData(UIMeshSection, RendererEntry.EmitterInstance, SpriteRenderer, ScaleFactor, LocationOffset, Alpha01, ParticleCountIncreaseAndDecrease);
	}
	else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(RendererEntry.RendererProperties))
	{
		AddRibbonRendererData(UIMeshSection, RendererEntry.EmitterInstance, RibbonRenderer, ScaleFactor, LocationOffset, Alpha01, ParticleCountIncreaseAndDecrease);
	}
}

FORCEINLINE MyVector2 FastRotate(const MyVector2 Vector, float Sin, float Cos)
{
	return MyVector2(Cos * Vector.X - Sin * Vector.Y,
		Sin * Vector.X + Cos * Vector.Y);
}
FORCEINLINE MyVector3 MakePositionVector(const MyVector2& InVector2D)
{
	return MyVector3(0, InVector2D.X, InVector2D.Y);
}

void ULGUIWorldParticleSystemComponent::AddSpriteRendererData(FLGUIMeshSection* UIMeshSection
	, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
	, UNiagaraSpriteRendererProperties* SpriteRenderer
	, float ScaleFactor, MyVector2 LocationOffset, float Alpha01
	, const int ParticleCountIncreaseAndDecrease
)
{
	FVector ComponentLocation = this->GetRelativeLocation();
	FVector ComponentScale = this->GetRelativeScale3D();
	FRotator ComponentRotation = this->GetRelativeRotation();
	float ComponentPitchRadians = FMath::DegreesToRadians(ComponentRotation.Pitch);

	FNiagaraDataSet& DataSet = EmitterInst->GetData();
	FNiagaraDataBuffer& ParticleData = DataSet.GetCurrentDataChecked();
	const int32 ParticleCount = ParticleData.GetNumInstances();

	int VertexCount = ParticleCount * 4;
	int IndexCount = ParticleCount * 6;
	auto& VertexData = UIMeshSection->vertices;
	auto& IndexData = UIMeshSection->triangles;

	const int VertexCountIncreaseAndDecrease = ParticleCountIncreaseAndDecrease * 4;
	const int IndexCountIncreaseAndDecrease = ParticleCountIncreaseAndDecrease * 6;

	int NewTotalVertexCount = ((VertexCount / VertexCountIncreaseAndDecrease) + (VertexCount % VertexCountIncreaseAndDecrease > 0 ? 1 : 0)) * VertexCountIncreaseAndDecrease;
	VertexData.SetNumZeroed(NewTotalVertexCount);

	int NewTotalIndexCount = ((IndexCount / IndexCountIncreaseAndDecrease) + (IndexCount % IndexCountIncreaseAndDecrease > 0 ? 1 : 0)) * IndexCountIncreaseAndDecrease;
	IndexData.SetNumZeroed(NewTotalIndexCount);
	if (IndexData.Num() > IndexCount)//set not required triangle index to zero
	{
		FMemory::Memzero(((uint8*)IndexData.GetData()) + IndexCount * sizeof(FLGUIIndexType), (IndexData.Num() - IndexCount) * sizeof(FLGUIIndexType));
	}

	if (ParticleCount < 1)
		return;

	bool LocalSpace = EmitterInst->GetCachedEmitter()->bLocalSpace;

	//const float FakeDepthScaler = 1 / WidgetProperties->FakeDepthScaleDistance;

	auto SubImageSize = (MyVector2)SpriteRenderer->SubImageSize;
	auto SubImageDelta = MyVector2::UnitVector / SubImageSize;

#if ENGINE_MAJOR_VERSION >= 5
	const auto PositionData = FNiagaraDataSetAccessor<FNiagaraPosition>::CreateReader(DataSet, SpriteRenderer->PositionBinding.GetDataSetBindableVariable().GetName());
#else
	const auto PositionData = FNiagaraDataSetAccessor<MyVector3>::CreateReader(DataSet, SpriteRenderer->PositionBinding.GetDataSetBindableVariable().GetName());
#endif
	const auto ColorData = FNiagaraDataSetAccessor<FLinearColor>::CreateReader(DataSet, SpriteRenderer->ColorBinding.GetDataSetBindableVariable().GetName());
	const auto VelocityData = FNiagaraDataSetAccessor<MyVector3>::CreateReader(DataSet, SpriteRenderer->VelocityBinding.GetDataSetBindableVariable().GetName());
	const auto SizeData = FNiagaraDataSetAccessor<MyVector2>::CreateReader(DataSet, SpriteRenderer->SpriteSizeBinding.GetDataSetBindableVariable().GetName());
	const auto RotationData = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, SpriteRenderer->SpriteRotationBinding.GetDataSetBindableVariable().GetName());
	const auto SubImageData = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, SpriteRenderer->SubImageIndexBinding.GetDataSetBindableVariable().GetName());
	const auto DynamicMaterialData = FNiagaraDataSetAccessor<MyVector4>::CreateReader(DataSet, SpriteRenderer->DynamicMaterialBinding.GetDataSetBindableVariable().GetName());

	auto GetParticlePosition2D = [&PositionData](int32 Index)
	{
		const auto Position3D = PositionData.GetSafe(Index, MyVector3::ZeroVector);
		return MyVector2(Position3D.X, Position3D.Z);
	};

	auto GetParticleDepth = [&PositionData](int32 Index)
	{
		return PositionData.GetSafe(Index, MyVector3::ZeroVector).Y;
	};

	auto GetParticleColor = [&ColorData](int32 Index)
	{
		return ColorData.GetSafe(Index, FLinearColor::White);
	};

	auto GetParticleVelocity2D = [&VelocityData](int32 Index)
	{
		const auto Velocity3D = VelocityData.GetSafe(Index, MyVector3::ZeroVector);
		return MyVector2(Velocity3D.X, -Velocity3D.Z);
	};

	auto GetParticleSize = [&SizeData](int32 Index)
	{
		return SizeData.GetSafe(Index, MyVector2::ZeroVector);
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
		return DynamicMaterialData.GetSafe(Index, MyVector4(0.f, 0.f, 0.f, 0.f));
	};

	for (int ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
	{
		auto ParticlePosition = GetParticlePosition2D(ParticleIndex) * ScaleFactor;
		auto ParticleSize = GetParticleSize(ParticleIndex) * ScaleFactor;

		if (LocalSpace)
		{
			ParticlePosition *= MyVector2(ComponentScale.X, ComponentScale.Z);
			ParticlePosition = ParticlePosition.GetRotated(-ComponentRotation.Pitch);
			ParticlePosition += LocationOffset;
			ParticlePosition += MyVector2(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;

			ParticleSize *= MyVector2(ComponentScale.X, ComponentScale.Z);
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


		const MyVector2 ParticleHalfSize = ParticleSize * 0.5;


		FColor ParticleColor = GetParticleColor(ParticleIndex).ToFColor(false);
		ParticleColor.A = ParticleColor.A * Alpha01;


		float ParticleRotationSin = 0, ParticleRotationCos = 0;

		if (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
		{
			const MyVector2 ParticleVelocity = GetParticleVelocity2D(ParticleIndex);

			ParticleRotationCos = MyVector2::DotProduct(ParticleVelocity.GetSafeNormal(), MyVector2(0.f, 1.f));
			const float SinSign = FMath::Sign(MyVector2::DotProduct(ParticleVelocity, MyVector2(1.f, 0.f)));

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

		MyVector2 TextureCoordinates[4];

		if (SubImageSize != MyVector2(1.f, 1.f))
		{
			const float ParticleSubImage = GetParticleSubImage(ParticleIndex);
			const int Row = (int)FMath::Floor(ParticleSubImage / SubImageSize.X) % (int)SubImageSize.Y;
			const int Column = (int)(ParticleSubImage) % (int)(SubImageSize.X);

			const float LeftUV = SubImageDelta.X * Column;
			const float Right = SubImageDelta.X * (Column + 1);
			const float TopUV = SubImageDelta.Y * Row;
			const float BottomUV = SubImageDelta.Y * (Row + 1);

			TextureCoordinates[0] = MyVector2(LeftUV, TopUV);
			TextureCoordinates[1] = MyVector2(Right, TopUV);
			TextureCoordinates[2] = MyVector2(LeftUV, BottomUV);
			TextureCoordinates[3] = MyVector2(Right, BottomUV);
		}
		else
		{
			TextureCoordinates[0] = MyVector2(0.f, 0.f);
			TextureCoordinates[1] = MyVector2(1.f, 0.f);
			TextureCoordinates[2] = MyVector2(0.f, 1.f);
			TextureCoordinates[3] = MyVector2(1.f, 1.f);
		}


		const auto MaterialData = GetDynamicMaterialData(ParticleIndex);

		MyVector2 PositionArray[4];
		PositionArray[0] = FastRotate(MyVector2(-ParticleHalfSize.X, -ParticleHalfSize.Y), ParticleRotationSin, ParticleRotationCos);
		PositionArray[1] = FastRotate(MyVector2(ParticleHalfSize.X, -ParticleHalfSize.Y), ParticleRotationSin, ParticleRotationCos);
		PositionArray[2] = -PositionArray[1];
		PositionArray[3] = -PositionArray[0];

		const int VertexIndex = ParticleIndex * 4;
		const int indexIndex = ParticleIndex * 6;


		for (int i = 0; i < 4; ++i)
		{
			VertexData[VertexIndex + i].Position = MakePositionVector(PositionArray[i] + ParticlePosition);
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

void ULGUIWorldParticleSystemComponent::AddRibbonRendererData(FLGUIMeshSection* UIMeshSection
	, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
	, UNiagaraRibbonRendererProperties* RibbonRenderer
	, float ScaleFactor, MyVector2 LocationOffset, float Alpha01
	, const int ParticleCountIncreaseAndDecrease
)
{
	FVector ComponentLocation = GetRelativeLocation();
	FVector ComponentScale = GetRelativeScale3D();
	FRotator ComponentRotation = GetRelativeRotation();

	FNiagaraDataSet& DataSet = EmitterInst->GetData();
	FNiagaraDataBuffer& ParticleData = DataSet.GetCurrentDataChecked();
	const int32 ParticleCount = ParticleData.GetNumInstances();

	auto& VertexData = UIMeshSection->vertices;
	auto& IndexData = UIMeshSection->triangles;
	VertexData.Reset();
	IndexData.Reset();

	if (ParticleCount < 2)
		return;

	const auto SortKeyReader = RibbonRenderer->SortKeyDataSetAccessor.GetReader(DataSet);

	const auto PositionData = RibbonRenderer->PositionDataSetAccessor.GetReader(DataSet);
	const auto ColorData = FNiagaraDataSetAccessor<FLinearColor>::CreateReader(DataSet, RibbonRenderer->ColorBinding.GetDataSetBindableVariable().GetName());
	const auto RibbonWidthData = RibbonRenderer->SizeDataSetAccessor.GetReader(DataSet);

	const auto RibbonFullIDData = RibbonRenderer->RibbonFullIDDataSetAccessor.GetReader(DataSet);

	auto GetParticlePosition2D = [&PositionData](int32 Index)
	{
		const auto Position3D = PositionData.GetSafe(Index, MyVector3::ZeroVector);
		return MyVector2(Position3D.X, Position3D.Z);
	};

	auto GetParticleColor = [&ColorData](int32 Index)
	{
		return ColorData.GetSafe(Index, FLinearColor::White);
	};

	auto GetParticleWidth = [&RibbonWidthData](int32 Index)
	{
		return RibbonWidthData.GetSafe(Index, 0.f);
	};

	auto AngleLargerThanPi = [](const MyVector2& A, const MyVector2& B)
	{
		float temp = A.X * B.Y - B.X * A.Y;
		return temp < 0;
	};

	auto GenerateLinePoint = [AngleLargerThanPi](const MyVector2& InCurrentPoint, const MyVector2& InPrevPoint, const MyVector2& InNextPoint
		, float InLineLeftWidth, float InLineRightWidth
		, MyVector2& OutPosA, MyVector2& OutPosB
		)
	{
		MyVector2 normalizedV1 = (InPrevPoint - InCurrentPoint).GetSafeNormal();
		MyVector2 normalizedV2 = (InNextPoint - InCurrentPoint).GetSafeNormal();
		if (normalizedV1 == -normalizedV2)
		{
			auto itemNormal = MyVector2(normalizedV2.Y, -normalizedV2.X);
			OutPosA = InCurrentPoint + InLineLeftWidth * itemNormal;
			OutPosB = InCurrentPoint - InLineRightWidth * itemNormal;
		}
		else
		{
			auto itemNormal = normalizedV1 + normalizedV2;
			itemNormal.Normalize();
			if (itemNormal.X == 0 && itemNormal.Y == 0)//wrong normal
			{
				itemNormal = MyVector2(normalizedV2.Y, -normalizedV2.X);
			}
			float prevDotN = MyVector2::DotProduct(normalizedV1, itemNormal);
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

	const int VertexCountIncreaseAndDecrease = ParticleCountIncreaseAndDecrease * 2;
	const int IndexCountIncreaseAndDecrease = ParticleCountIncreaseAndDecrease * 6;

	auto AddRibbonVerts = [&](TArray<int32>& RibbonIndices, int32& InOutVertexCount, int32& InOutIndexCount)
	{
		const int32 numParticlesInRibbon = RibbonIndices.Num();
		if (numParticlesInRibbon < 3)
			return;

		int VertexCount = (numParticlesInRibbon - 1) * 2;
		int IndexCount = (numParticlesInRibbon - 2) * 6;

		auto CurrentVertexIndex = InOutVertexCount;
		auto CurrentIndexIndex = InOutIndexCount;
		InOutVertexCount += VertexCount;
		InOutIndexCount += IndexCount;

		int NewTotalVertexCount = ((InOutVertexCount / VertexCountIncreaseAndDecrease) + (InOutVertexCount % VertexCountIncreaseAndDecrease > 0 ? 1 : 0)) * VertexCountIncreaseAndDecrease;
		VertexData.SetNumZeroed(NewTotalVertexCount);

		int NewTotalIndexCount = ((InOutIndexCount / IndexCountIncreaseAndDecrease) + (InOutIndexCount % IndexCountIncreaseAndDecrease > 0 ? 1 : 0)) * IndexCountIncreaseAndDecrease;
		IndexData.SetNumZeroed(NewTotalIndexCount);

		const int32 StartDataIndex = RibbonIndices[0];

		float TotalDistance = 0.0f;

		MyVector2 LastPosition = GetParticlePosition2D(StartDataIndex);
		MyVector2 CurrentPosition = MyVector2::ZeroVector;
		float CurrentWidth = 0.f;
		MyVector2 LastToCurrentVector = MyVector2::ZeroVector;
		float LastToCurrentSize = 0.f;
		float LastU0 = 0.f;
		float LastU1 = 0.f;

		MyVector2 LastParticleUIPosition = LastPosition * ScaleFactor;

		if (LocalSpace)
		{
			LastParticleUIPosition *= MyVector2(ComponentScale.X, ComponentScale.Z);
			LastParticleUIPosition = LastParticleUIPosition.GetRotated(-ComponentRotation.Pitch);
			LastParticleUIPosition += LocationOffset;

			LastParticleUIPosition += MyVector2(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;
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

		MyVector2 InitialPositionArray[2];
		InitialPositionArray[0] = LastToCurrentVector.GetRotated(90.f) * InitialWidth * 0.5f;
		InitialPositionArray[1] = -InitialPositionArray[0];

		for (int i = 0; i < 2; ++i)
		{
			VertexData[CurrentVertexIndex + i].Position = MakePositionVector(InitialPositionArray[i] + LastParticleUIPosition);
			VertexData[CurrentVertexIndex + i].Color = InitialColor;
			VertexData[CurrentVertexIndex + i].TextureCoordinate[0] = MyVector2(i, 0);
		}

		CurrentVertexIndex += 2;

		int32 NextIndex = CurrentIndex + 1;

		while (NextIndex < numParticlesInRibbon)
		{
			const int32 NextDataIndex = RibbonIndices[NextIndex];
			const MyVector2 NextPosition = GetParticlePosition2D(NextDataIndex);
			MyVector2 CurrentToNextVector = NextPosition - CurrentPosition;
			const float CurrentToNextSize = CurrentToNextVector.Size();
			CurrentWidth = GetParticleWidth(CurrentDataIndex) * ScaleFactor;
			FColor CurrentColor = GetParticleColor(CurrentDataIndex).ToFColor(false);
			CurrentColor.A = CurrentColor.A * Alpha01;

			// Normalize CurrToNextVec
			CurrentToNextVector *= 1.f / CurrentToNextSize;

			const MyVector2 CurrentTangent = (LastToCurrentVector + CurrentToNextVector).GetSafeNormal();

			TotalDistance += LastToCurrentSize;

			MyVector2 CurrentPositionArray[2];
			CurrentPositionArray[0] = CurrentTangent.GetRotated(90.f) * CurrentWidth * 0.5f;
			CurrentPositionArray[1] = -CurrentPositionArray[0];

			MyVector2 CurrentParticleUIPosition = CurrentPosition * ScaleFactor;

			if (LocalSpace)
			{
				CurrentParticleUIPosition *= MyVector2(ComponentScale.X, ComponentScale.Z);
				CurrentParticleUIPosition = CurrentParticleUIPosition.GetRotated(-ComponentRotation.Pitch);
				CurrentParticleUIPosition += LocationOffset;
				CurrentParticleUIPosition += MyVector2(ComponentLocation.X, ComponentLocation.Z) * ScaleFactor;
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

			MyVector2 TextureCoordinates0[2];
			TextureCoordinates0[0] = MyVector2(CurrentU0, 1.f);
			TextureCoordinates0[1] = MyVector2(CurrentU0, 0.f);

			MyVector2 TextureCoordinates1[2];
			TextureCoordinates1[0] = MyVector2(CurrentU1, 1.f);
			TextureCoordinates1[1] = MyVector2(CurrentU1, 0.f);

			for (int i = 0; i < 2; ++i)
			{
				VertexData[CurrentVertexIndex + i].Position = MakePositionVector(CurrentPositionArray[i] + CurrentParticleUIPosition);
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
	};

	if (!MultiRibbons)
	{
		TArray<int32> SortedIndices;
		for (int32 i = 0; i < ParticleCount; ++i)
		{
			SortedIndices.Add(i);
		}

		SortedIndices.Sort([&SortKeyReader](const int32& A, const int32& B) {	return (SortKeyReader[A] < SortKeyReader[B]); });

		int VertexCount = 0, IndexCount = 0;
		AddRibbonVerts(SortedIndices, VertexCount, IndexCount);
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

			int VertexCount = 0, IndexCount = 0;
			for (TPair<FNiagaraID, TArray<int32>>& Pair : MultiRibbonSortedIndices)
			{
				TArray<int32>& SortedIndices = Pair.Value;
				SortedIndices.Sort([&SortKeyReader](const int32& A, const int32& B) {	return (SortKeyReader[A] < SortKeyReader[B]); });
				AddRibbonVerts(SortedIndices, VertexCount, IndexCount);
			}
			if (IndexData.Num() > IndexCount)
			{
				FMemory::Memzero(((uint8*)IndexData.GetData()) + IndexCount * sizeof(FLGUIIndexType), (IndexData.Num() - IndexCount) * sizeof(FLGUIIndexType));
			}
		}
	}
}
//PRAGMA_ENABLE_OPTIMIZATION