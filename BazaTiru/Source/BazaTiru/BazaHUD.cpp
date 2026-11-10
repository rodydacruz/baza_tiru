#include "BazaHUD.h"

#include "BazaBossBase.h"
#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaPowerUp.h"
#include "BazaSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	/** stage.tscn: theme_override_colors/font_color on both labels. */
	const FLinearColor LabelColor(0.435294f, 1.f, 0.733333f, 1.f);
	const FLinearColor ShadowColor(0.184314f, 0.121569f, 0.278431f, 1.f);
	const FLinearColor AlertColor(1.f, 0.24f, 0.36f, 1.f);
}

void ABazaHUD::DrawHUD()
{
	Super::DrawHUD();

	const ABazaGameMode* Stage = ABazaGameMode::Get(this);
	if (!Stage || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;

	// The labels were laid out in the 320x180 viewport, so everything scales from
	// there, letterboxed the same way the camera is.
	const float Scale = FMath::Min(
		Canvas->SizeX / BazaSettings::ScreenWidth,
		Canvas->SizeY / BazaSettings::ScreenHeight);

	const float OffsetX = (Canvas->SizeX - BazaSettings::ScreenWidth * Scale) * 0.5f;
	const float OffsetY = (Canvas->SizeY - BazaSettings::ScreenHeight * Scale) * 0.5f;

	// Godot drew these at font size 16; the engine's large font is close enough
	// that three quarters of the scale lands on the same height.
	const float TextScale = Scale * 0.75f;

	auto DrawLabel = [&](const FString& Text, const FLinearColor& Colour, float GameX, float GameY, bool bCentred)
	{
		float Width = 0.f;
		float Height = 0.f;
		GetTextSize(Text, Width, Height, Font, TextScale);

		const float X = OffsetX + GameX * Scale - (bCentred ? Width * 0.5f : 0.f);
		const float Y = OffsetY + GameY * Scale;

		// stage.tscn gave both labels a drop shadow.
		DrawText(Text, ShadowColor, X + Scale, Y + Scale, Font, TextScale);
		DrawText(Text, Colour, X, Y, Font, TextScale);
	};

	auto DrawBar = [&](float GameX, float GameY, float GameW, float GameH, const FLinearColor& Colour)
	{
		DrawRect(Colour,
			OffsetX + GameX * Scale, OffsetY + GameY * Scale,
			GameW * Scale, GameH * Scale);
	};

	DrawLabel(FString::Printf(TEXT("SCORE: %d"), Stage->GetScore()), LabelColor, 113.f, 6.f, false);

	// Lives, as pips rather than a number, so the eye can count them mid-fight.
	for (int32 Index = 0; Index < Stage->GetLives(); ++Index)
	{
		DrawBar(6.f + Index * 8.f, 8.f, 5.f, 5.f, LabelColor);
	}

	// Running power-ups, top right: a name in the pickup's colour and a bar that
	// drains as it runs out.
	if (const ABazaPlayer* Ship = Stage->GetPlayer())
	{
		const float SmallScale = Scale * 0.45f;
		float Row = 5.f;

		for (const EBazaPowerUp Type : { EBazaPowerUp::Shield, EBazaPowerUp::Speed, EBazaPowerUp::Magnet, EBazaPowerUp::Spread })
		{
			const float Remaining = Ship->GetPowerUpRemaining(Type);
			const float Duration = BazaPowerUps::DurationOf(Type);
			if (Remaining <= 0.f || Duration <= 0.f)
			{
				continue;
			}

			const FLinearColor Colour = BazaPowerUps::ColourOf(Type);
			const float X = OffsetX + 250.f * Scale;
			const float Y = OffsetY + Row * Scale;

			DrawText(BazaPowerUps::LabelOf(Type), ShadowColor, X + Scale * 0.5f, Y + Scale * 0.5f, Font, SmallScale);
			DrawText(BazaPowerUps::LabelOf(Type), Colour, X, Y, Font, SmallScale);

			DrawBar(250.f, Row + 8.f, 62.f, 2.f, ShadowColor);
			DrawBar(250.f, Row + 8.f, 62.f * (Remaining / Duration), 2.f, Colour);

			Row += 13.f;
		}
	}

	if (const ABazaBossBase* Boss = Stage->GetBoss())
	{
		if (Boss->IsEntering())
		{
			// Blinking, because a boss walking on deserves a beat of warning.
			const float Blink = FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.5f);
			if (Blink > 0.25f)
			{
				DrawLabel(TEXT("!!  WARNING  !!"), AlertColor,
					BazaSettings::ScreenWidth * 0.5f, 60.f, true);
			}
		}
		else if (Boss->GetPhase() != EBazaPhase::Defeated)
		{
			DrawLabel(Boss->DisplayName.ToString(), AlertColor, 90.f, 160.f, false);

			// A proper bar along the bottom, which the 2D version never had: its
			// ProgressBar was pinned to the hull and vanished with the boss.
			DrawBar(90.f, 174.f, 140.f, 4.f, ShadowColor);
			DrawBar(90.f, 174.f, 140.f * Boss->GetHealthFraction(), 4.f, AlertColor);
		}
	}

	if (Stage->IsGameOver())
	{
		DrawLabel(TEXT("BU PANHA MOU - DA ENTER PO BAI DI NOVU"), LabelColor,
			BazaSettings::ScreenWidth * 0.5f, 85.f, true);
	}

	if (!Stage->bShowDebug)
	{
		return;
	}

	// F1 overlay: the counters that decide what spawns next, and the boss state
	// machine, which is otherwise impossible to watch from the outside.
	DrawLabel(FString::Printf(TEXT("EVENTS: %d"), Stage->GetEvents()), LabelColor, 6.f, 20.f, false);

	if (const ABazaBossBase* Boss = Stage->GetBoss())
	{
		const TCHAR* PhaseName = TEXT("?");
		switch (Boss->GetPhase())
		{
		case EBazaPhase::Entering:  PhaseName = TEXT("ENTERING"); break;
		case EBazaPhase::Appear:    PhaseName = TEXT("APEAR");    break;
		case EBazaPhase::Hide:      PhaseName = TEXT("HIDE");     break;
		case EBazaPhase::AllIn:     PhaseName = TEXT("ALL IN");   break;
		case EBazaPhase::Defeated:  PhaseName = TEXT("DEFEATED"); break;
		}

		DrawLabel(FString::Printf(TEXT("BOSS %s  HP %.0f  HITS %d"),
			PhaseName, Boss->GetHealth(), Boss->GetStrikeCount()), LabelColor, 6.f, 34.f, false);
	}
	else
	{
		DrawLabel(TEXT("BOSS: NOT ON FIELD  (F3)"), LabelColor, 6.f, 34.f, false);
	}
}
