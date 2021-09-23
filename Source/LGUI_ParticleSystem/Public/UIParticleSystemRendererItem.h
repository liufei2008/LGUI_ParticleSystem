// Copyright 2021 LexLiu. All Rights Reserved.

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
	virtual void SetClipType(ELGUICanvasClipType clipType)override;
	virtual void SetRectClipParameter(const FVector4& OffsetAndSize, const FVector4& Feather)override;
	virtual void SetTextureClipParameter(UTexture* ClipTex, const FVector4& OffsetAndSize)override;
	virtual void SetDrawcallMesh(UUIDrawcallMesh* InUIDrawcallMesh)override;

	UPROPERTY(Transient) class UMaterialInstanceDynamic* RectClipDynamicMaterial = nullptr;
	UPROPERTY(Transient) class UMaterialInstanceDynamic* TextureClipDynamicMaterial = nullptr;
	TWeakObjectPtr<UMaterialInterface> Material = nullptr;
};

UCLASS(Transient)
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
