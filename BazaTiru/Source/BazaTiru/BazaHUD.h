// Basa Tiru - the score and retry labels from stage.tscn's "ui" CanvasLayer.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BazaHUD.generated.h"

UCLASS()
class BAZATIRU_API ABazaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
