#include "BazaPlayerController.h"

#include "BazaGameMode.h"
#include "BazaSfx.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

ABazaPlayerController::ABazaPlayerController()
{
	bShowMouseCursor = false;

	// The game mode points the view at its own fixed camera.
	bAutoManageActiveCameraTarget = false;
}

void ABazaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
}

void ABazaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsInputKeyDown(EKeys::Escape))
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
		return;
	}

	ABazaGameMode* Stage = ABazaGameMode::Get(this);
	if (!Stage)
	{
		return;
	}

	// stage.gd's unwritten executeCheatCode(), so the later phases can be reached
	// without playing up to them first.
	if (WasInputKeyJustPressed(EKeys::F1))
	{
		Stage->bShowDebug = !Stage->bShowDebug;
	}
	if (WasInputKeyJustPressed(EKeys::F2))
	{
		Stage->CheatAddScore(25);
	}
	if (WasInputKeyJustPressed(EKeys::F3))
	{
		Stage->CheatSpawnBoss();
	}
	if (WasInputKeyJustPressed(EKeys::F4))
	{
		Stage->CheatSkipBossPhase();
	}
	if (WasInputKeyJustPressed(EKeys::F5))
	{
		Stage->CheatSpawnPowerUp();
	}

	if (WasInputKeyJustPressed(EKeys::M))
	{
		if (UBazaSfx* Sfx = UBazaSfx::Get(this))
		{
			Sfx->ToggleMute();
		}
	}

	if (!Stage->IsGameOver())
	{
		return;
	}

	if (IsInputKeyDown(EKeys::Enter))
	{
		// stage.gd reloaded stage.tscn; here that is just the level we are in.
		if (const UWorld* World = GetWorld())
		{
			UGameplayStatics::OpenLevel(this, FName(*World->GetName()));
		}
	}
}
