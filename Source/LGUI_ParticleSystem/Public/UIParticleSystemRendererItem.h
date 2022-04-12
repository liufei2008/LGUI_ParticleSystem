// Copyright 2021-present LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ActorComponent/UIDirectMeshRenderable.h"
#include "Core/Actor/UIBaseActor.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "UIParticleSystemRendererItem.generated.h"

class ULGUIWorldParticleSystemComponent;

UCLASS()
class LGUI_PARTICLESYSTEM_API UUIParticleSystemRendererItem : public UUIDirectMeshRenderable
{
	GENERATED_BODY()

public:
	UUIParticleSystemRendererItem(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	TWeakObjectPtr<class UUIParticleSystem> Manager = nullptr;
	virtual void SetMaterial(UMaterialInterface* InMaterial);
protected:
	virtual void OnMeshDataReady()override;
	virtual bool HaveValidData()const override;
	virtual UMaterialInterface* GetMaterial()const override;

	TWeakObjectPtr<UMaterialInterface> Material = nullptr;
};

UCLASS(ClassGroup = LGUI, Transient, NotPlaceable)
class LGUI_PARTICLESYSTEM_API AUIParticleSystemRendererItemActor : public AUIBaseActor
{
	GENERATED_BODY()

public:
	AUIParticleSystemRendererItemActor();

	FORCEINLINE virtual UUIItem* GetUIItem()const override { return UIParticleSystemRendererItem; }
	FORCEINLINE UUIParticleSystemRendererItem* GetUIParticleSystemRendererItem()const { return UIParticleSystemRendererItem; }
private:
	UPROPERTY(Category = "LGUI", VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
		class UUIParticleSystemRendererItem* UIParticleSystemRendererItem;

};
