# Asteroid Survivor

A top-down arcade-style space shooter built with **Unreal Engine 5**.
Survive increasingly difficult waves of asteroids – shoot them down before they collide with your ship!

---

## Gameplay

| Action | Keyboard | Controller |
|--------|----------|------------|
| Thrust forward | `W` / `↑` | Left stick ↑ |
| Brake / reverse | `S` / `↓` | Left stick ↓ |
| Rotate left | `A` / `←` | Left stick ← |
| Rotate right | `D` / `→` | Left stick → |
| Fire | `Space` / `LMB` | Face button (A/Cross) |

- **Asteroids** drift in from the edges of the screen.
- **Large** asteroids split into 2 **Medium** asteroids when destroyed.
- **Medium** asteroids split into 2 **Small** asteroids when destroyed.
- **Small** asteroids are fully destroyed.
- Each wave spawns more (and faster) asteroids.
- You start with **3 lives**. Colliding with or being hit by an asteroid costs a life.
- The game ends when all lives are lost.

---

## Scoring

| Asteroid | Points |
|----------|--------|
| Large    | 20     |
| Medium   | 50     |
| Small    | 100    |

---

## Project Structure

```
AsteroidSurvivor/
├── AsteroidSurvivor.uproject          # Unreal project descriptor (Engine 5.3)
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   └── DefaultInput.ini
└── Source/
    ├── AsteroidSurvivor.Target.cs     # Game target
    ├── AsteroidSurvivorEditor.Target.cs
    └── AsteroidSurvivor/
        ├── AsteroidSurvivor.Build.cs
        ├── AsteroidSurvivor.cpp                     # Module entry point
        ├── AsteroidSurvivorGameMode.h/cpp           # Wave management, scoring, lives
        ├── AsteroidSurvivorPlayerController.h/cpp   # Input mode setup
        ├── AsteroidSurvivorShip.h/cpp               # Player-controlled ship
        ├── AsteroidSurvivorProjectile.h/cpp         # Bullet fired by ship
        ├── AsteroidSurvivorAsteroid.h/cpp           # Asteroid actor with split logic
        ├── AsteroidSurvivorAsteroidSpawner.h/cpp    # Wave-based asteroid spawner
        └── AsteroidSurvivorHUD.h/cpp                # Canvas-drawn score / lives / wave HUD
```

---

## Requirements

- **Unreal Engine 5.3** (download via the Epic Games Launcher)
- A C++ compiler compatible with UE5 (Visual Studio 2022 on Windows, Xcode on macOS)

---

## Getting Started

1. Clone this repository.
2. Right-click `AsteroidSurvivor.uproject` → **Generate Visual Studio project files**.
3. Open the generated `.sln` in Visual Studio and build the **Development Editor** configuration.
4. Open `AsteroidSurvivor.uproject` in Unreal Editor.
5. Create the required levels (see [Level Setup](#level-setup) below).
6. Assign the game mode and press **Play** to start the game.

---

## Level Setup

This repository ships **only C++ source and configuration files** — no `.umap` level files are included.
When you first open the project in Unreal Editor you must create the gameplay level yourself.
Without proper setup the viewport will appear **completely black** (the only visible elements will be HUD text drawn by the C++ code).

### Creating the GameLevel map

1. **File → New Level** and choose **Empty Level** (or **Basic** if available).
2. **File → Save Current Level As…** and save it to `Content/Maps/GameLevel`.
   > The `DefaultGame.ini` already references `/Game/Maps/GameLevel` as the server default map, so the name and path must match exactly.
3. Repeat the process to create `Content/Maps/MainMenu` if you need a separate main-menu map.

### Adding essential environment actors

An empty level has **no lighting, no sky, and no geometry**, which is why the viewport is black. Add the following actors to your `GameLevel` map:

| What to add | How | Why |
|-------------|-----|-----|
| **Directional Light** | Place Actors panel → **Lights → Directional Light**. Drag it into the level. | Provides the main sunlight for the scene. |
| **Sky Atmosphere** | Place Actors panel → **Visual Effects → Sky Atmosphere** | Renders a procedural sky so the background is not black. |
| **Sky Light** | Place Actors panel → **Lights → Sky Light** | Provides ambient/indirect lighting from the sky. Set **Real Time Capture** to `true` so it updates with the atmosphere. |
| **Exponential Height Fog** *(optional)* | Place Actors panel → **Visual Effects → Exponential Height Fog** | Adds atmospheric depth; not strictly necessary for a top-down game but improves the look. |
| **Floor / ground plane** *(optional)* | Place Actors panel → **Basic → Plane** or **Cube**, scale it up, and position it below the play area. | Gives the camera something to see. For a space theme you can skip this and use a dark background or star-field material. |

> **Tip:** If you prefer a quick start, choose **File → New Level → Basic** instead of **Empty Level** — the Basic template already includes a directional light, sky atmosphere, sky light, and a floor.

### Assigning the Game Mode

1. Open `GameLevel` in the editor.
2. Go to **World Settings** (Window → World Settings if the panel is not visible).
3. Under **Game Mode**, set **GameMode Override** to `AsteroidSurvivorGameMode` (or your `BP_AsteroidSurvivorGameMode` Blueprint subclass if you created one).
4. Verify that **Default Pawn Class** is set to `AsteroidSurvivorShip` (or your `BP_Ship` Blueprint) and **HUD Class** is set to `AsteroidSurvivorHUD` (or `BP_HUD`).

### Making it the default map

If you want the editor to open this map automatically:

1. **Edit → Project Settings → Maps & Modes**.
2. Set **Editor Startup Map** to `/Game/Maps/GameLevel`.
3. Set **Game Default Map** to `/Game/Maps/GameLevel` (or `/Game/Maps/MainMenu` if you have a separate menu level).

After completing these steps, press **Play** and you should see the ship, asteroids, and the HUD.

### Blueprint subclasses (recommended)

The C++ base classes expose all tuneable properties via `UPROPERTY(EditDefaultsOnly)`.
Create Blueprint subclasses for each actor to assign meshes, materials and audio:

| C++ base class | Suggested Blueprint name |
|----------------|--------------------------|
| `AAsteroidSurvivorShip` | `BP_Ship` |
| `AAsteroidSurvivorProjectile` | `BP_Projectile` |
| `AAsteroidSurvivorAsteroid` | `BP_Asteroid` |
| `AAsteroidSurvivorAsteroidSpawner` | `BP_AsteroidSpawner` |
| `AAsteroidSurvivorGameMode` | `BP_AsteroidSurvivorGameMode` |
| `AAsteroidSurvivorHUD` | `BP_HUD` |

---

## Troubleshooting

### The viewport is completely black (I can only see HUD text)

This happens when the level has **no lighting or sky actors**. The HUD (score, lives, wave) is drawn directly to the screen via C++ Canvas rendering, so it is visible even in an empty level.

**Fix:** Follow the [Level Setup](#level-setup) instructions above to add a Directional Light, Sky Atmosphere, and Sky Light to your level. Alternatively, create a new level using the **Basic** template instead of **Empty Level**.

### I don't see the ship or asteroids

1. Make sure the **Game Mode** is set correctly in World Settings (see [Assigning the Game Mode](#assigning-the-game-mode)).
2. Verify that the **Default Pawn Class** is `AsteroidSurvivorShip` (or your `BP_Ship` Blueprint subclass).
3. If you are using Blueprint subclasses, ensure you have assigned a **Static Mesh** to the ship and asteroid Blueprints. The C++ classes create mesh components but do not assign a mesh asset — that is your responsibility in the Blueprint (or you can set a mesh in C++ if you prefer).
4. Check that the camera is positioned correctly. The ship creates its own top-down camera via a Spring Arm; make sure the Spring Arm length and camera rotation are sensible (defaults are set in C++).

### The level won't load or the editor opens an untitled map

The `DefaultGame.ini` references `/Game/Maps/MainMenu` and `/Game/Maps/GameLevel`. If those maps do not exist, the editor may fall back to an untitled empty level. Create and save the maps at the exact paths listed above.

---

## License

This project is provided as-is for educational and portfolio purposes.