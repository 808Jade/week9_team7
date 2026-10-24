#include "EnginePCH.h"
#include "Color.h"

static const FColor White(255, 255, 255, 255);
static const FColor Black(0, 0, 0, 255);
static const FColor Transparent(0, 0, 0, 0);
static const FColor Red(255, 0, 0, 255);
static const FColor Green(0, 255, 0, 255);
static const FColor Blue(0, 0, 255, 255);
static const FColor Yellow(255, 255, 0, 255);
static const FColor Cyan(0, 255, 255, 255);
static const FColor Magenta(255, 0, 255, 255);
static const FColor Orange(255, 165, 0, 255);
static const FColor Purple(128, 0, 128, 255);
static const FColor Turquoise(0, 255, 255, 255);
static const FColor Silver(192, 192, 192, 255);
static const FColor Emerald(0, 128, 0, 255);


FColor::FColor(uint32 InColor)
    :Bits(InColor)
{
}

