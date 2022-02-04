// Copyright 2021-present LexLiu. All Rights Reserved.

#pragma once

#include "Widgets/SLeafWidget.h"

/**
 * 
 */
class LGUI_PARTICLESYSTEM_API SLGUIParticleSystemUpdateAgentWidget : public SLeafWidget
{	
public:
	SLATE_BEGIN_ARGS(SLGUIParticleSystemUpdateAgentWidget)
	{		
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) {};
	
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier)const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		OnPaintCallbackDelegate.ExecuteIfBound();
		return LayerId;
	}
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override {}

	FSimpleDelegate OnPaintCallbackDelegate;
};
