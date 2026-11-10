# Basa Tiru — Godot → Unreal C++

The Godot 4.6 project in `scenes/`, `scripts/` and `sprites/` is untouched and still
runs. The Unreal 5.8 rewrite lives in `BazaTiru/`, in C++.

## Running it

The whole game is built in code — no level to open, no blueprints. `BazaGameMode`
spawns the camera, the light, the backdrop, the player and every enemy, so the
project boots straight into an empty engine map (`/Engine/Maps/Entry`) and starts
playing.

Open `BazaTiru/BazaTiru.uproject` and press Play.

| Key | |
| --- | --- |
| Arrows | move; holding left also turns the ship to face left, and letting go turns it back |
| Space | shoot |
| Enter | restart, once you have lost |
| Escape | quit |
| F1 | debug overlay: event counter, boss phase, boss health |
| F2 | +25 score |
| F3 | call the next boss down now |
| F4 | skip the current boss to its next phase |
| F5 | drop the next kind of power-up just ahead of the ship |
| M | mute |

F1–F3 are `stage.gd`'s `executeCheatCode()`, which the original declared and left
empty.

## How the 2D maps onto 3D

The ship still moves on a flat plane, and every gameplay number is still in Godot's
320×180 pixels. Only one function knows about 3D, in `BazaSettings.h`:

    Godot X  →  Unreal  X   (screen right)
    Godot Y  →  Unreal −Z   (Godot Y points down, Unreal Z points up)
                Unreal  Y   is depth, and stays at 0 for anything that collides

`ABazaActor` keeps a `GamePosition` in those pixels and derives the real world
transform from it. That is why the ported movement code still reads like the `.gd`
file it came from — `Position.X -= MoveSpeed * DeltaSeconds` is the same line it
was, with the same 100 in it. One pixel is 10 Unreal units.

Anything behind the play plane — the nebula, the parallax stars — goes through
`PlaceAtDepth`, which pushes it out from the screen centre and scales it up by
exactly enough to cover the same area it would have covered up front.

## Where each script went

| Godot                     | Unreal                                      |
| ------------------------- | ------------------------------------------- |
| `scripts/settings.gd`     | `BazaSettings.h`                            |
| `scripts/stage.gd`        | `BazaGameMode` + `BazaHUD` + `BazaPlayerController` |
| `scripts/player.gd`       | `BazaPlayer`                                |
| `scripts/shot.gd`         | `BazaShot`                                  |
| `scripts/tiro.gd`         | `BazaCanonBall`                             |
| `scripts/asteroid.gd`     | `BazaAsteroid`                              |
| `scripts/lowney.gd`       | `BazaLowEnemy`                              |
| `scripts/dark.gd`         | `BazaDark`                                  |
| `scripts/boss_nemy_01.gd` | `BazaBossBase` + `BazaBossNemy01`           |
| `scripts/explosion.gd`    | `BazaExplosion`                             |
| `scripts/background.gd`   | `BazaBackground`                            |
| `scenes/t_warning.gd`     | `BazaWarning`                               |

And the concepts:

| Godot                    | Unreal                                        |
| ------------------------ | --------------------------------------------- |
| `Node2D` / `Area2D`      | `AActor` with an overlap-only collision component |
| `_process(delta)`        | `Tick(DeltaSeconds)`                          |
| `Timer` node             | `FTimerHandle` + `GetWorldTimerManager()`     |
| `signal` / `emit_signal` | `DECLARE_DYNAMIC_MULTICAST_DELEGATE` / `Broadcast` |
| `preload` + `instantiate`| `SpawnActor<T>`, wrapped as `ABazaGameMode::SpawnAt` |
| groups (`is_in_group`)   | `IsA<>()` / `Cast<>()` — checked by type now  |
| `queue_free()`           | `Destroy()`                                   |
| `get_parent()` (a stage child) | `ABazaGameMode::Get(this)`              |
| `Label` in a `CanvasLayer` | `AHUD::DrawText`                            |

## The bosses

`ABazaBossBase` owns everything they share: health, the phase enum, the health bar,
the hit reaction, the entrance sweep, the death sequence, the attack timer and the
score payout. A subclass is only that one boss's pattern.

**Nemy01** is the ported one. It closes in and tracks the player's row, and at 70%
health it breaks off, runs out past the left edge and keeps lobbing shots back —
that is where the warning markers come from — then turns around and charges.

In the original that charge was the end of it: the boss parked at x=60, turned round
and did nothing for the rest of the fight. Now it holds the left side facing the
ship and gets aggressive. It works the rows and fires, with a three-ball fan every
third volley. Every 2.6 seconds it flashes white and draws back for half a second —
that is the tell — then rams along the line to where the ship was, and eases back
to its spot. It holds fire during the tell and the ram, so the attack reads. This
is the fight the player turns round for.

**Spelhu** ("mirror") is new. It is the ship's *echo*: it records where the ship
goes and when it fires, and plays it back two seconds later — same row, mirrored
into the right-hand side so it faces the ship, and every volley the ship fired comes
back at it along that row. Its eye lights just before an echo volley. The fight is
about leading it: shoot at the row it is about to arrive on, then be somewhere else
when your own volley comes back. At 60% health the echo closes to 1.5 seconds; at
30% to one second, and each echo volley gains the spread power-up's side bolts.

**Barku Mai** ("mother ship") is the last boss, and taller than the screen: it
creeps on from the right with the camera rumbling and closes that side like a wall.
Flying into the hull costs a life. Only the core in the middle of its face takes
damage, and only while its shutters are open; bolts anywhere else spark off the
armour, except on the four turrets, which can be shot off (eight hits, 50 points)
and stop firing. The whole ship heaves slowly up and down, so the core never holds
still. It fires aimed balls from the turrets in turn, and every few seconds a
curtain of balls down the full height with one gap in it. Every 6.5 seconds it
rams: red stripes flash across the half it is about to take, it slides up or down
so one edge of the hull sits just short of the middle line, then charges the full
width of the screen. The other half is always left open, and it goes for the half
the ship is in seven times out of ten. At 66% the hangars open and gunships come
out in pairs; at 33% the core throws a sweeping three-stream spray, the rams come
every 3.8 seconds and sometimes go straight back in down the other half.

A new boss is one class and one line of data:

```cpp
class ABazaBossNemy03 : public ABazaBossBase
{
    ABazaBossNemy03();                                  // ConfigureBoss + its meshes
    virtual void UpdateMovement(float DeltaSeconds) override;
    virtual void OnAttackTick() override;
    virtual void OnDamaged() override;                  // phase changes on health
};
```

`ConfigureBoss(TotalHealth, HitRadiusPx, HealthBarOffsetPx)` in the constructor sets
the rest; `SetAttackInterval` changes the firing rate mid-fight. Then add it to
`BossRoster` and `BossScoreGates` in `ABazaGameMode`'s constructor — currently
`{ Nemy01, Spelhu, Barku Mai }` at `{ 100, 450, 1000 }` points. Each one arrives once, the first
time the field is clear past its own gate.

## Art

The original sprites are imported into `Content/Art/` as `T_*` textures, at nearest
filter with no mips and no block compression, so the pixels stay hard-edged. Two
unlit materials go with them: `M_BazaBackdrop` (opaque) and `M_BazaSprite` (masked).
They were generated by a script, since a material is not a text file — see the note
at the end.

`sprites/background.png` is the backdrop, scrolling at its original 30 px/s and
wrapping every screen width. It is 640 px wide — two screens — so the seam never
shows.

The 16×16 sprites were left out on purpose: flat billboards would undo the 3D. The
ships and rocks are generated instead.

### Generated meshes

`BazaMeshBuilder.h` is a small flat-shaded builder — every triangle carries its own
vertices and its own normal, so edges stay hard and the silhouette reads at 320×180.
It has `AddTri`, `AddQuad`, `AddSlab` (extrude a flat outline into a wing or a fin)
and `AddPrism` (engine pods, gun barrels), plus `AddTriFacingOut`, which orients a
face away from a point so hull winding never has to be worked out by hand.

**`BazaRockMesh`** builds the asteroids: an icosphere, subdivided once, pushed about
by four smooth lobes in random directions, then stretched unevenly per axis and
roughened. Summing sines of the dot product against each lobe axis is what keeps the
surface varying smoothly all the way round with no seam, which per-vertex noise
alone would not manage. Each asteroid seeds from its own id, so they are plainly the
same kind of thing and plainly not the same one.

**`BazaShipMesh`** builds every ship from one parametrised hull: faceted fuselage
with a raised canopy and a keel, swept wings, an optional dorsal fin and engine
pods. `FBazaShipParams` sets length, span, depth, sweep, nose length and which way
it faces. The player, the gunships and BossNemy01's hull are all the same builder
with different numbers.

The builder works in ship space — X nose, Y canopy, Z wingspan — which lands exactly
on this port's mapping with no rotation: the nose points screen-right, the wings run
screen-vertical and the canopy faces the camera. That is the top-down view a shmup
wants.

### Lighting

One directional light, plus an ambient floor in the material. `M_BazaShape` feeds a
fraction of each object's own colour into emissive, so a face turned away from the
light falls to a dim version of itself rather than to black — without it, a faceted
rock has solid black holes punched through it. A second directional light would do
the same job but makes the renderer complain about which one owns forward shading,
and the floor is more predictable.

## Power-ups

Every 4–7 seconds a pickup drifts in from the right, slower than the rocks and
bobbing, swinging about the vertical so its 3D shape shows. Fly into it to take it.
The shapes are generated with the same mesh builder as the ships (`BazaPowerUp.cpp`).

| Pickup | Shape | Effect |
| --- | --- | --- |
| Shield | cyan hexagon crest | 10 s of a hexagonal ring round the ship that eats incoming bolts and cannon balls |
| Speed | yellow double chevron | 10 s of 1.5× movement and **double points** |
| Repair | green gear | lives back to full |
| Magnet | red horseshoe with silver tips | 12 s of bolts turned into small pink orbs that bend towards the nearest enemy ahead of them |
| Spread | orange fan of arrowheads | 12 s of two extra bolts angled out at ±14° |

Timed effects stack with each other, show on the HUD top right with a draining bar,
and are all lost with a life. The shield ring flickers through its last two seconds.
Shield and repair are weighted up, since they are what keeps a run going. The
weights are shield 6, speed 3, magnet 2, spread 2, and repair 8 — but repair only
while a life is missing. So with lives full, nearly half the drops are shields;
down a life, repair is the most likely pick (38%) and shield next (29%). `PowerUpMinInterval` and
`PowerUpMaxInterval` on the game mode set the timing.

Magnet orbs steer at up to 420°/s towards the closest enemy within 150 px that is in
front of them, so they curve in rather than snap, and never loop back for something
already passed.

## Sound

The Godot game was silent. Every effect here is synthesised in code by `UBazaSfx`
(`BazaSfx.cpp`) — no audio files. Each clip is rendered once, on the first sound
of a run, from the two things an NES sound chip had: pulse waves and
sample-and-hold noise. The amplitude is then stepped down to 48 levels, which is
where the 8-bit crunch comes from.

| Sound | When | Made of |
| --- | --- | --- |
| Shoot | the player fires a pair | soft low "pew": mostly triangle sliding 620 → 240 Hz, a touch of square, a noise snap at the front |
| EnemyShot | a gunship or dark fires | square, lower and duller than the player's |
| Cannon | a boss volley | low thump with noise in it |
| Hit | a shot lands on something that survives | short tick |
| ExplosionSmall / Big | picked by explosion size | noise whose hold period lengthens as it dies, so it falls in pitch; the big one adds a sub rumble |
| PlayerHit | losing a life | long wobbling fall |
| Respawn | back in | rising C–E–G–C |
| BossWarning | a boss starts its entrance | two-tone siren under the WARNING banner |
| BossDefeated | a boss finally goes | five-note fanfare |
| GameOver | out of lives | four notes down |
| PowerUp | a pickup is taken | two quick upward sweeps, the second an octave up |

Anything can play one with `UBazaSfx::Play(this, EBazaSfx::Shoot)`. The same sound
cannot restart within a few hundredths of a second, so a volley of bolts makes one
report. Past 20 voices the small sounds are dropped and only the cues get through.
Non-cue sounds also get a slight random pitch so repeats do not sound stamped out.

Tuning happens in `EnsureRendered`: each sound is one short block of arithmetic.
`MasterVolume` scales everything.

## The 8-bit look

Set in `Config/DefaultEngine.ini`, under `[SystemSettings]`. The scene renders at
about 320×180 and is blown up with nearest-neighbour upscaling, so the pixels stay
hard; anti-aliasing, bloom, motion blur and the rest are off so nothing smooths
them back out.

- Chunkier or smoother: change `r.ScreenPercentage`. It is a percentage of the
  window, so 16.7 of a 1920×1080 window is 320×180.
- Off entirely: `r.ScreenPercentage=100`.

Colour is still 24-bit. Quantising to a real fixed palette needs a post-process
material, which has to be built in the editor's material graph.

## Bugs in the original, and what was done about them

**Fixed, because they made content unreachable:**

- **The boss almost never appeared.** `stage.gd` gated it on
  `score > 100 && score < 200`, which only fires if the score happens to sit in that
  window at the moment the field clears. The darks paid a point per hit and could
  not be killed, so the score usually tore past 200 while they were still up — and
  then neither branch ever fired again and the boss was gone for the run. It is
  latched now: it arrives once, the first time the field is clear past 100.
- **The player could not die.** `player.gd` spawns an explosion on a hit, but its
  `queue_free()` and its `destroyed` signal are both commented out, so the retry
  screen `stage.gd` builds was unreachable. There are three lives now. Tick
  `ABazaGameMode::bPlayerIsInvulnerable` to play it the old way, with no fail state.
- **Gunships leaked.** `lowney.gd` never freed itself, so every one that survived
  kept ticking and firing forever off-screen. They despawn at the asteroids'
  boundary now.

- **Bolts flew through enemies.** `shot.gd` meant to stop on gunships, but
  `low_nemy.tscn` never joins the `low_nemy` group, so the check never fired: one
  bolt could kill a whole row, and bolts sailed straight through the boss. A bolt
  now spends itself on the first asteroid, gunship, dark or boss it touches.
- **Gunships and darks were harmless to touch.** `player.gd` only reacted to
  asteroids, shots, cannon balls and the boss. Flying into a gunship or a dark now
  costs a life and destroys it too.
- **Nemy01 went idle after its charge** — see the bosses section.

**Kept, because they are how the game plays:**

- **Asteroids still pass through gunships.** `asteroid.gd` checked for the same
  missing group; making rocks and gunships collide would clear the field in ways
  the original never did.
- **Cannon balls never despawn on contact.** `tiro.gd` has an
  `_on_shot_area_entered`, but `canon_ball.tscn` never wires the signal to it.
- **Cannon ball speed depends on distance.** The boss assigns the raw vector to the
  player without normalising it or applying a speed, so a shot from across the
  screen travels several times faster than a close one. It is the fight's character.
- **`events = +1`** in `stage.gd` assigns one instead of adding one. It only runs
  when `events` is already zero, so it makes no difference.

## What was added

Optics:

- Hits flash the target white; kills shake the camera, scaled to what died.
- Explosions throw tumbling shards and fade, instead of one still frame.
- The player ship banks into its turns and its thruster breathes.
- Asteroids tumble on their own axes; darks turn; the boss has a spinning eye.
- A parallax starfield in front of the nebula. `StarCount = 0` turns it off.
- Lives are drawn as pips, and a boss gets a name and a health bar along the bottom.

Mechanics:

- **Three lives**, with a respawn and two seconds of blinking invulnerability.
- **Bosses arrive.** They sweep in from off-screen behind a blinking WARNING, rather
  than appearing mid-field. This is the `EBazaPhase::Entering` state, which the
  original did not have.
- **Bosses die properly.** `DEFEATED` was declared in `settings.gd` and never used;
  a boss now comes apart over a second and a half of blasts and pays out 250.
- **Darks can be killed** — three hits. They still score and shoot back on every
  hit, as before, but no longer have to be waited out.
- **Gunships weave and lead their shots**, though only while the player is still
  ahead of them; one that has been passed still fires straight.

## Timers: bind them to the object

The world timer manager belongs to the game instance, and the game instance
outlives the level. A timer set with a lambda that captures `this` is not tied to
the object, so if the object goes before the timer does — restarting the level with
a boss on the field is enough — it fires into freed memory and takes the editor down
with an access violation. That happened here once, in the boss's attack timer.

So:

- Set timers as `SetTimer(Handle, this, &AClass::Method, ...)`, which the timer
  manager drops by itself when the object dies; or, if it has to be a lambda, wrap
  it in `FTimerDelegate::CreateWeakLambda(this, ...)`.
- `ABazaActor::EndPlay` clears every timer left on the actor anyway, as a backstop.

## Notes on the structure

Two things are deliberately closer to the Godot original than to normal Unreal
practice, because the point was a faithful port:

- **The player is an `AActor`, not an `APawn`,** and nothing is possessed — it was a
  plain child of `stage.tscn`. Input is polled with
  `APlayerController::IsInputKeyDown`, which is what `Input.is_key_pressed` did. A
  ground-up Unreal game would use an `APawn` with Enhanced Input.
- **Positions live in game pixels,** not world units, for the reason above.

### Regenerating the art

`PythonScriptPlugin` is enabled so the assets can be scripted. Two scripts live in
`Tools/`: `import_art.py` brings the sprites in as pixel-art textures and makes the
backdrop and sprite materials, and `make_shape_material.py` builds `M_BazaShape`,
whose `Ambient` value is the lighting floor described above.

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" BazaTiru\BazaTiru.uproject -run=pythonscript -script="Tools\make_shape_material.py" -unattended
```

### Capturing a frame

`-BazaShot=<seconds>` plays for that long, writes a PNG to `Saved/Screenshots` and
quits. `-BazaBoss` calls the first boss down at once, `-BazaBoss=2` the second, `-BazaBoss=3` the mother ship, and
`-BazaGod` keeps the ship alive so the frame shows the fight rather than the retry
screen, and `-BazaSkip=N` moves the boss on N phases first (`-BazaSkip=3` lands
Nemy01 straight in its final phase). `-BazaDrops` lays one of each pickup out in a
row, `-BazaPowered` starts the ship with every timed power-up, and `-BazaAutoFire`
holds the trigger down so the shots show. `-BazaRestartAfter=N` reloads the level
once, N seconds in, as Enter does after a game over — run it with `-BazaBoss` to
check the restart path that once crashed (see below). Give `-BazaShot` a value of
1 or more:
Unreal reads `-BazaShot=0.8` as a map named `.8` and fails to load. These run through the editor binary, so build `BazaTiruEditor` first.

```
UnrealEditor-Cmd.exe BazaTiru\BazaTiru.uproject -game -windowed -ResX=1280 -ResY=720 -BazaShot=8
```
