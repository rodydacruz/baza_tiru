#include "BazaGameMode.h"

#include "BazaAsteroid.h"
#include "BazaBackground.h"
#include "BazaBossNemy01.h"
#include "BazaBossNemy02.h"
#include "BazaBossNemy03.h"
#include "BazaDark.h"
#include "BazaHUD.h"
#include "BazaLowEnemy.h"
#include "BazaPlayer.h"
#include "BazaPlayerController.h"
#include "BazaPowerUp.h"
#include "BazaSfx.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogBazaTiru, Log, All);

ABazaGameMode::ABazaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	// The player is a plain stage actor here, exactly as player.tscn was a plain
	// child of stage.tscn, so no pawn gets possessed and the view stays on the
	// fixed camera the way a 2D viewport would.
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ABazaPlayerController::StaticClass();
	HUDClass = ABazaHUD::StaticClass();

	// stage.gd only ever knew about one boss. The roster keeps that as data, so a
	// new boss is a class plus a line here.
	BossRoster = { ABazaBossNemy01::StaticClass(), ABazaBossNemy02::StaticClass(), ABazaBossNemy03::StaticClass() };
	BossScoreGates = { 100, 450, 1000 };
}

ABazaGameMode* ABazaGameMode::Get(const UObject* WorldContext)
{
	return Cast<ABazaGameMode>(UGameplayStatics::GetGameMode(WorldContext));
}

void ABazaGameMode::BeginPlay()
{
	Super::BeginPlay();

	Lives = StartingLives;

	EnsureCamera();

	if (bSpawnDefaultScenery)
	{
		SpawnScenery();
	}

	Player = SpawnAt<ABazaPlayer>(ABazaPlayer::StartPosition);

	// stage.tscn also had one asteroid sitting on the field at startup.
	SpawnAt<ABazaAsteroid>(FVector2D(304.795f, 29.928f));

	// stage.tscn: spawn_timer, autostart, Godot's default 1 second, repeating.
	GetWorldTimerManager().SetTimer(
		SpawnTimer, this, &ABazaGameMode::OnSpawnTimerTimeout, 1.f, true);

	SchedulePowerUp();

	SetUpCaptureMode();
}

ABazaPlayer* ABazaGameMode::GetPlayer() const
{
	return Player.Get();
}

void ABazaGameMode::SchedulePowerUp()
{
	GetWorldTimerManager().SetTimer(
		PowerUpTimer, this, &ABazaGameMode::OnPowerUpTimer,
		FMath::FRandRange(PowerUpMinInterval, PowerUpMaxInterval), false);
}

void ABazaGameMode::OnPowerUpTimer()
{
	if (bIsGameOver)
	{
		return;
	}

	SpawnPowerUp(PickPowerUp(), FVector2D(
		BazaSettings::ScreenWidth + 12.f,
		FMath::FRandRange(24.f, BazaSettings::ScreenHeight - 24.f)));

	SchedulePowerUp();
}

EBazaPowerUp ABazaGameMode::PickPowerUp() const
{
	struct FWeighted
	{
		EBazaPowerUp Type;
		int32 Weight;
	};

	const FWeighted Table[] = {
		// Shield and repair are weighted up on purpose: they are what keeps a run
		// alive, and the game leans on them being around.
		{ EBazaPowerUp::Shield, 6 },
		{ EBazaPowerUp::Speed,  3 },
		{ EBazaPowerUp::Magnet, 2 },
		{ EBazaPowerUp::Spread, 2 },
		{ EBazaPowerUp::Repair, Lives < StartingLives ? 8 : 0 },
	};

	int32 Total = 0;
	for (const FWeighted& Entry : Table)
	{
		Total += Entry.Weight;
	}

	int32 Roll = FMath::RandRange(0, Total - 1);
	for (const FWeighted& Entry : Table)
	{
		if (Roll < Entry.Weight)
		{
			return Entry.Type;
		}
		Roll -= Entry.Weight;
	}

	return EBazaPowerUp::Shield;
}

ABazaPowerUp* ABazaGameMode::SpawnPowerUp(EBazaPowerUp Type, const FVector2D& Where)
{
	ABazaPowerUp* Pickup = SpawnAt<ABazaPowerUp>(Where);
	if (Pickup)
	{
		Pickup->Configure(Type);
	}
	return Pickup;
}

void ABazaGameMode::CheatSpawnPowerUp()
{
	const EBazaPowerUp Type = static_cast<EBazaPowerUp>(NextCheatPowerUp);
	NextCheatPowerUp = (NextCheatPowerUp + 1) % static_cast<int32>(EBazaPowerUp::Count);

	// Just ahead of the ship; it drifts back into it.
	const FVector2D ShipPosition = GetPlayerPosition();
	SpawnPowerUp(Type, FVector2D(ShipPosition.X + 50.f, ShipPosition.Y));
}

void ABazaGameMode::SetUpCaptureMode()
{
	// Development aid, off unless asked for on the command line:
	//   -BazaShot=6            play for six seconds, save a shot, quit
	//   -BazaShot=6 -BazaBoss  call the boss down first, to capture the fight
	// Shots land in BazaTiru/Saved/Screenshots.
	float CaptureAfter = 0.f;
	if (!FParse::Value(FCommandLine::Get(), TEXT("BazaShot="), CaptureAfter))
	{
		return;
	}

	// -BazaGod keeps the ship alive, so a capture shows the fight rather than the
	// retry screen. The debug overlay stays on F1 either way.
	if (FParse::Param(FCommandLine::Get(), TEXT("BazaGod")))
	{
		bPlayerIsInvulnerable = true;
	}

	// -BazaBoss calls the first boss down; -BazaBoss=2 skips to the second.
	int32 BossNumber = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("BazaBoss="), BossNumber))
	{
		BossesSpawned = FMath::Clamp(BossNumber - 1, 0, FMath::Max(BossRoster.Num() - 1, 0));
		CheatSpawnBoss();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("BazaBoss")))
	{
		CheatSpawnBoss();
	}

	// -BazaDrops lays one of every pickup out in a row, to capture their shapes.
	if (FParse::Param(FCommandLine::Get(), TEXT("BazaDrops")))
	{
		const int32 Count = static_cast<int32>(EBazaPowerUp::Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			SpawnPowerUp(static_cast<EBazaPowerUp>(Index), FVector2D(120.f + Index * 44.f, 40.f));
		}
	}

	// -BazaSkip=N moves the boss on N phases straight away, to capture a late one.
	int32 Skips = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("BazaSkip="), Skips))
	{
		for (int32 Index = 0; Index < Skips; ++Index)
		{
			CheatSkipBossPhase();
		}
	}

	// -BazaRestartAfter=N reloads the level once, N seconds in, the way Enter does
	// after a game over. With a boss on the field this is the path that used to
	// crash, so it stays here as a regression check.
	static bool bHasRestarted = false;
	float RestartAfter = 0.f;
	if (!bHasRestarted && FParse::Value(FCommandLine::Get(), TEXT("BazaRestartAfter="), RestartAfter))
	{
		FTimerHandle RestartHandle;
		GetWorldTimerManager().SetTimer(
			RestartHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bHasRestarted = true;
				UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
			}),
			RestartAfter, false);
	}

	FTimerHandle CaptureHandle;
	// Weak lambdas, so neither fires into a game mode that has already gone.
	GetWorldTimerManager().SetTimer(
		CaptureHandle, FTimerDelegate::CreateWeakLambda(this, []()
		{
			FScreenshotRequest::RequestScreenshot(
				TEXT("BazaShot"), /*bInShowUI=*/true, /*bAddFilenameSuffix=*/false);
		}),
		CaptureAfter, false);

	FTimerHandle QuitHandle;
	GetWorldTimerManager().SetTimer(
		QuitHandle, FTimerDelegate::CreateWeakLambda(this, []() { FPlatformMisc::RequestExit(false); }),
		CaptureAfter + 2.f, false);
}

void ABazaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Camera)
	{
		return;
	}

	// Settle the camera back to where it belongs after a knock.
	Shake = FMath::FInterpTo(Shake, 0.f, DeltaSeconds, 7.f);

	if (Shake > 0.1f)
	{
		const FVector Offset(
			FMath::FRandRange(-Shake, Shake),
			0.f,
			FMath::FRandRange(-Shake, Shake));

		Camera->SetActorLocation(CameraHome + Offset);
	}
	else
	{
		Camera->SetActorLocation(CameraHome);
	}
}

void ABazaGameMode::AddShake(float Amount)
{
	Shake = FMath::Min(Shake + Amount, 90.f);
}

void ABazaGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	EnsureCamera();

	if (NewPlayer && Camera)
	{
		NewPlayer->bAutoManageActiveCameraTarget = false;
		NewPlayer->SetViewTarget(Camera);
	}
}

float ABazaGameMode::ComputeCameraDistance() const
{
	const float HalfWidth = BazaSettings::Px(BazaSettings::ScreenWidth * 0.5f);
	return HalfWidth / FMath::Tan(FMath::DegreesToRadians(CameraFov * 0.5f));
}

void ABazaGameMode::EnsureCamera()
{
	UWorld* World = GetWorld();
	if (!World || Camera)
	{
		return;
	}

	// Pull the camera back just far enough that the 320x180 field fills the frame.
	const float Distance = ComputeCameraDistance();

	const FVector2D Centre(BazaSettings::ScreenWidth * 0.5f, BazaSettings::ScreenHeight * 0.5f);
	CameraHome = BazaSettings::ToWorld(Centre) + FVector(0.f, Distance, 0.f);

	// Looking back down -Y puts screen-right on +X and screen-up on +Z, which is
	// the orientation every ported coordinate assumes.
	const FRotator Rotation(0.f, -90.f, 0.f);

	Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraHome, Rotation);
	if (!Camera)
	{
		UE_LOG(LogBazaTiru, Error, TEXT("Could not spawn the stage camera."));
		return;
	}

	if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(CameraFov);

		// Letterbox to 16:9 so the framing matches Godot's "viewport" stretch mode
		// no matter what shape the window is.
		CameraComponent->SetConstraintAspectRatio(true);
		CameraComponent->SetAspectRatio(BazaSettings::ScreenWidth / BazaSettings::ScreenHeight);
	}

	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->bAutoManageActiveCameraTarget = false;
		Controller->SetViewTarget(Camera);
	}
}

void ABazaGameMode::SpawnScenery()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ABazaBackground* Background = SpawnAt<ABazaBackground>(FVector2D::ZeroVector))
	{
		// Everything behind the play plane has to know how far the camera is, so
		// it can scale itself up to cover the frame.
		Background->FitToCamera(ComputeCameraDistance());
	}

	// One key light, aimed into the screen from above and to the right so the faces
	// the camera can see are the faces that get lit. The fill comes from the
	// material's own ambient floor rather than a second light: two directional
	// lights make the renderer complain about which one owns forward shading, and
	// the floor does the job more predictably anyway.
	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-38.f, -68.f, 0.f));

	if (Light)
	{
		Light->SetMobility(EComponentMobility::Movable);

		if (ULightComponent* LightComponent = Light->GetLightComponent())
		{
			LightComponent->SetIntensity(1.5f);
			LightComponent->SetLightColor(FLinearColor(1.f, 0.97f, 0.93f));
			LightComponent->SetCastShadows(false);
		}
	}
}

FVector2D ABazaGameMode::GetPlayerPosition() const
{
	if (Player)
	{
		return Player->GetGamePosition();
	}

	// The boss aims at this every half second, so a dead player should not drag
	// its shots to the top-left corner of the screen.
	return FVector2D(BazaSettings::ScreenWidth * 0.5f, BazaSettings::ScreenHeight * 0.5f);
}

bool ABazaGameMode::ConsumeLife()
{
	Lives = FMath::Max(0, Lives - 1);
	return Lives > 0;
}

void ABazaGameMode::OnPlayerDestroyed()
{
	bIsGameOver = true;
	Lives = 0;

	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(PowerUpTimer);

	UBazaSfx::Play(this, EBazaSfx::GameOver);
}

void ABazaGameMode::OnBossDefeated()
{
	// Frees the field again, so the stage can carry on spawning.
	Events = 0;
	Boss = nullptr;
}

void ABazaGameMode::ReleaseEventSlot()
{
	Events = FMath::Max(0, Events - 1);
}

void ABazaGameMode::OnSpawnTimerTimeout()
{
	if (bIsGameOver)
	{
		return;
	}

	SpawnAt<ABazaAsteroid>(FVector2D(
		BazaSettings::ScreenWidth + 8.f,
		FMath::FRandRange(0.f, BazaSettings::ScreenHeight)));

	// One spawn in five also brings a gunship.
	if (FMath::RandRange(0, 4) == 0)
	{
		SpawnAt<ABazaLowEnemy>(FVector2D(
			BazaSettings::ScreenWidth + 16.f,
			FMath::FRandRange(20.f, BazaSettings::ScreenHeight - 20.f)));
	}
}

void ABazaGameMode::HandleScore(int32 Points, int32 EnemyId)
{
	// The speed power-up doubles everything scored while it runs.
	const int32 Multiplier = Player ? Player->GetScoreMultiplier() : 1;
	Score += Points * Multiplier;

	UE_LOG(LogBazaTiru, Verbose, TEXT("Enemy defeated with ID: %d  +%d points"), EnemyId, Points);

	RunScorePhase();
}

void ABazaGameMode::RunScorePhase()
{
	// Nothing new arrives while a scripted encounter is still on the field.
	if (Events > 0)
	{
		return;
	}

	if (Score > 20 && Score < 100)
	{
		// A field of darks, spread out beyond the right edge.
		for (int32 Index = 0; Index < 12; ++Index)
		{
			++Events;

			SpawnAt<ABazaDark>(FVector2D(
				FMath::FRandRange(BazaSettings::ScreenWidth + 8.f, 2.f * BazaSettings::ScreenWidth),
				FMath::FRandRange(0.f, BazaSettings::ScreenHeight)));
		}
	}

	// stage.gd gated the boss on "score > 100 && score < 200", which only lands if
	// the score happens to sit inside that window at the moment the field clears.
	// It rarely does: the score tears past 200 while the darks are still up, and
	// then neither branch ever fires again and the boss is unreachable for the
	// rest of the run. Latched instead, so each boss arrives once, the first time
	// the field is clear past its own score gate.
	if (BossScoreGates.IsValidIndex(BossesSpawned) && Score > BossScoreGates[BossesSpawned])
	{
		CheatSpawnBoss();
	}
}

void ABazaGameMode::CheatAddScore(int32 Points)
{
	HandleScore(Points, 0);
}

void ABazaGameMode::CheatSkipBossPhase()
{
	if (ABazaBossBase* Current = Boss.Get())
	{
		Current->DebugSkipPhase();
	}
}

void ABazaGameMode::CheatSpawnBoss()
{
	UWorld* World = GetWorld();
	if (!World || Boss.IsValid() || !BossRoster.IsValidIndex(BossesSpawned))
	{
		return;
	}

	TSubclassOf<ABazaBossBase> BossClass = BossRoster[BossesSpawned];
	if (!BossClass)
	{
		return;
	}

	++BossesSpawned;
	Events = 1;

	// Starts off the right edge and sweeps in, so the arrival is an event.
	const FVector2D Where(
		BazaSettings::ScreenWidth + 60.f,
		FMath::FRandRange(40.f, BazaSettings::ScreenHeight - 40.f));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABazaBossBase* Spawned = World->SpawnActor<ABazaBossBase>(
		BossClass, BazaSettings::ToWorld(Where), FRotator::ZeroRotator, Params);

	if (Spawned)
	{
		Spawned->SetGamePosition(Where);
		Spawned->OnScore.AddDynamic(this, &ABazaGameMode::HandleScore);
		Boss = Spawned;
	}
}
