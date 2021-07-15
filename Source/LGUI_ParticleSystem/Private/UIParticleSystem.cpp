// Copyright 2021 LexLiu. All Rights Reserved.

#include "UIParticleSystem.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Engine.h"
#include "LGUIWorldParticleSystemComponent.h"
#include "UIParticleSystemRendererItem.h"
#include "Core/LGUIMesh/UIDrawcallMesh.h"
#include "SLGUIParticleSystemUpdateAgentWidget.h"

#define LOCTEXT_NAMESPACE "UIParticleSystem"

UUIParticleSystem::UUIParticleSystem(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUIParticleSystem::BeginPlay()
{
	Super::BeginPlay();

	if (!UpdateAgentWidget.IsValid())
	{
		UpdateAgentWidget = SNew(SLGUIParticleSystemUpdateAgentWidget);
		GEngine->GameViewport->AddViewportWidgetContent(UpdateAgentWidget.ToSharedRef());
		UpdateAgentWidget->OnPaintCallbackDelegate.BindUObject(this, &UUIParticleSystem::OnPaintUpdate);
	}
	if (IsValid(ParticleSystem))
	{
		auto WorldParticleSystemActor = this->GetWorld()->SpawnActor<ALGUIParticleSystemActor>();
#if WITH_EDITOR
		WorldParticleSystemActor->SetActorLabel(FString(TEXT("LGUI_PS_")) + this->GetOwner()->GetActorLabel());
#endif
		ParticleSystemInstance = WorldParticleSystemActor->Emit(ParticleSystem, bAutoActivateParticleSystem);

		if (bAutoActivateParticleSystem)
		{
			SetRenderEntries();
		}
	}
}

void UUIParticleSystem::SetRenderEntries()
{
	if (!RenderEntriesValid)
	{
		UWorld* World = this->GetWorld();
		if (World)
		{
			RenderEntries = ParticleSystemInstance->GetRenderEntries();
			RenderEntriesValid = true;
			for (int i = 0; i < RenderEntries.Num(); i++)
			{
				auto ParticleSytemRendererItemActor = World->SpawnActor<AUIParticleSystemRendererItemActor>();
				auto RendererItem = ParticleSytemRendererItemActor->GetUIParticleSystemRendererItem();
				RendererItem->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				RendererItem->SetWidth(0);
				RendererItem->SetHeight(0);
				RendererItem->SetDepth(this->GetDepth() + i);
				RendererItem->SetMaterial(RenderEntries[i].Material);
				RendererItem->Manager = this;
				UIParticleSystemRenderers.Add(RendererItem);
			}
		}
	}
}
void UUIParticleSystem::ActivateParticleSystem(bool Reset)
{
	if (ParticleSystemInstance.IsValid())
	{
		ParticleSystemInstance->Activate(Reset);
		SetRenderEntries();
	}
}

void UUIParticleSystem::DeactivateParticleSystem()
{
	if (ParticleSystemInstance.IsValid())
		ParticleSystemInstance->Deactivate();
}
/**
 * 为什么分开两处更新(Tick中更新网格，Slate的OnPaint中读取粒子数据)？
 * 如果都放到Tick里，那么Ribbon读取的数据是错乱的；如果都放到OnPaint里，那么Sprite的UIMesh会有内存溢出；还不清楚原因。
 * 估计是由于多线程造成的，OnPaint和ParticleSystem中的Tick和默认的Tick不是在一个线程里。
 */ 
void UUIParticleSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ParticleSystemInstance.IsValid())
	{
		//update transform
		auto rootUIItem = this->GetRenderCanvas()->GetUIItem();
		auto rootSpaceLocation = rootUIItem->GetComponentTransform().InverseTransformPosition(this->GetComponentLocation());
		auto rootSpaceLocation2D = FVector2D(rootSpaceLocation.X, rootSpaceLocation.Y);
		ParticleSystemInstance->SetTransformationForUIRendering(rootSpaceLocation2D, FVector2D(this->GetRelativeScale3D()), this->GetRelativeRotation().Yaw);
		//update mesh
		if (IsUIActiveInHierarchy())
		{
			for (int i = 0; i < RenderEntries.Num(); i++)
			{
				auto UIDrawcallMesh = UIParticleSystemRenderers[i]->GetDrawcallMesh();
				if (UIDrawcallMesh != nullptr)
				{
					UIDrawcallMesh->GenerateOrUpdateMesh(false, 1);
				}
			}
		}
	}
}
void UUIParticleSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (ParticleSystemInstance.IsValid())
	{
		auto WorldParticleActor = ParticleSystemInstance->GetOwner();
		if (IsValid(WorldParticleActor))
		{
			WorldParticleActor->Destroy();
		}
		ParticleSystemInstance.Reset();
	}
	RenderEntries.Empty();
	for (auto item : UIParticleSystemRenderers)
	{
		if (IsValid(item))
		{
			auto itemActor = item->GetOwner();
			if (IsValid(itemActor))
			{
				itemActor->Destroy();
			}
		}
	}
	UIParticleSystemRenderers.Empty();
	if (UpdateAgentWidget.IsValid())
	{
		if (IsValid(GEngine) && IsValid(GEngine->GameViewport))
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(UpdateAgentWidget.ToSharedRef());
		}
	}
}
void UUIParticleSystem::ApplyUIActiveState()
{
	Super::ApplyUIActiveState();
}

DECLARE_CYCLE_STAT(TEXT("UIParticleSystem RenderToUI"), STAT_UIParticleSystem, STATGROUP_LGUI);
void UUIParticleSystem::OnPaintUpdate()
{
	//Read particle data in slate OnPaint
	if (IsUIActiveInHierarchy())
	{
		if (ParticleSystemInstance.IsValid())
		{
			SCOPE_CYCLE_COUNTER(STAT_UIParticleSystem);
			//auto layoutScale = this->GetRootCanvas()->GetCanvasScale();
			auto layoutScale = 1.0f;
			//auto locationOffset = FVector2D(-rootUIItem->GetWidth() * 0.5f, -rootUIItem->GetHeight() * 0.5f);
			auto locationOffset = FVector2D::ZeroVector;
			for (int i = 0; i < RenderEntries.Num(); i++)
			{
				auto UIDrawcallMesh = UIParticleSystemRenderers[i]->GetDrawcallMesh();
				if (UIDrawcallMesh != nullptr)
				{
					ParticleSystemInstance->RenderUI(UIDrawcallMesh, RenderEntries[i], layoutScale, locationOffset, bUseAlpha ? this->GetFinalAlpha01() : 1.0f);
				}
			}
		}
	}
}

#if WITH_EDITOR
void UUIParticleSystem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
void UUIParticleSystem::SetUseAlpha(bool value)
{
	if (bUseAlpha != value)
	{
		bUseAlpha = value;
	}
}
void UUIParticleSystem::SetParticleSystemTemplate(UNiagaraSystem* value)
{
	if (ParticleSystem != value)
	{
		ParticleSystem = value;
		if (ParticleSystemInstance.IsValid())
		{
			ParticleSystemInstance->SetAsset(ParticleSystem);
			ParticleSystemInstance->ResetSystem();
		}
	}
}

UMaterialInterface* UUIParticleSystem::GetNormalMaterial(UMaterialInterface* keyMaterial)
{
	if (auto valueMaterialPtr = NormalMaterialMap.Find(keyMaterial))
	{
		return *valueMaterialPtr;
	}
	return nullptr;
}
UMaterialInterface* UUIParticleSystem::GetRectClipMaterial(UMaterialInterface* keyMaterial)
{
	if (auto valueMaterialPtr = RectClipMaterialMap.Find(keyMaterial))
	{
		return *valueMaterialPtr;
	}
	return nullptr;
}
UMaterialInterface* UUIParticleSystem::GetTextureClipMaterialMap(UMaterialInterface* keyMaterial)
{
	if (auto valueMaterialPtr = TextureClipMaterialMap.Find(keyMaterial))
	{
		return *valueMaterialPtr;
	}
	return nullptr;
}


AUIParticleSystemActor::AUIParticleSystemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystem = CreateDefaultSubobject<UUIParticleSystem>(TEXT("UIParticleSystem"));
	RootComponent = UIParticleSystem;
}

#undef LOCTEXT_NAMESPACE
