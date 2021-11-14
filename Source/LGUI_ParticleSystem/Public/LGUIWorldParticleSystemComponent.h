// Copyright 2021 LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "LGUIWorldParticleSystemComponent.generated.h"


struct FLGUIMeshSection;
class FNiagaraEmitterInstance;
class UNiagaraSpriteRendererProperties;
class UNiagaraRibbonRendererProperties;

struct FLGUINiagaraRendererEntry
{
	FLGUINiagaraRendererEntry(UNiagaraRendererProperties* PropertiesIn, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstIn, UNiagaraEmitter* EmitterIn, UMaterialInterface* MaterialIn)
		: RendererProperties(PropertiesIn), EmitterInstance(EmitterInstIn), Emitter(EmitterIn), Material(MaterialIn) {}

	UNiagaraRendererProperties* RendererProperties;
	TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstance;
	UNiagaraEmitter* Emitter;
	UMaterialInterface* Material;
};

UCLASS()
class LGUI_PARTICLESYSTEM_API ULGUIWorldParticleSystemComponent : public UNiagaraComponent
{
    GENERATED_BODY()
public:
	TArray<FLGUINiagaraRendererEntry> GetRenderEntries();

    void SetTransformationForUIRendering(FVector2D Location, FVector2D Scale, float Angle);

    void RenderUI(TWeakPtr<FLGUIMeshSection> UIMeshSection, FLGUINiagaraRendererEntry RendererEntry, float ScaleFactor, FVector2D LocationOffset, float Alpha01);
private:
    void AddSpriteRendererData(TWeakPtr<FLGUIMeshSection> UIMeshSection, int32 MaxParticleCount
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraSpriteRendererProperties* SpriteRenderer
		, float ScaleFactor, FVector2D LocationOffset, float Alpha01
	);
    void AddRibbonRendererData(TWeakPtr<FLGUIMeshSection> UIMeshSection, int32 MaxParticleCount
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraRibbonRendererProperties* RibbonRenderer
		, float ScaleFactor, FVector2D LocationOffset, float Alpha01
	);
};

UCLASS()
class LGUI_PARTICLESYSTEM_API ALGUIParticleSystemActor : public AActor
{
	GENERATED_BODY()

public:
    ALGUIParticleSystemActor();

	ULGUIWorldParticleSystemComponent* Emit(UNiagaraSystem* NiagaraSystemTemplate, bool AutoActivate);
	UPROPERTY(VisibleAnywhere, Transient)
		ULGUIWorldParticleSystemComponent* Niagara;
};
