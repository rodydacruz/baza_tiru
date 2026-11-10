// Basa Tiru - the stage's global keys. Ported from stage.gd's _input(event).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BazaPlayerController.generated.h"

UCLASS()
class BAZATIRU_API ABazaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABazaPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
};
