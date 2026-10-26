#include "EnginePCH.h"
#include "ShapeComponent.h"

UShapeComponent::UShapeComponent()
{
	SetCurrentCollisionResponse(ECollisionResponse::Block);
	bCastShadow = false;
	bReceiveShadow = false;
}