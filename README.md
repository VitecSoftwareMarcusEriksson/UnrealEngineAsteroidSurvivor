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
├── AsteroidSurvivor.uproject          # Unreal project descriptor (Engine 5.7)
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   └── DefaultInput.ini
└── Source/
    ├── AsteroidSurvivor.Target.cs     # Game target
    ├── AsteroidSurvivorEditor.Target.cs
    ├── AsteroidSurvivor/
    │   ├── AsteroidSurvivor.Build.cs
    │   ├── AsteroidSurvivor.cpp                     # Module entry point
    │   ├── AsteroidSurvivorGameMode.h/cpp           # Wave management, scoring, lives
    │   ├── AsteroidSurvivorPlayerController.h/cpp   # Input mode setup
    │   ├── AsteroidSurvivorShip.h/cpp               # Player-controlled ship
    │   ├── AsteroidSurvivorProjectile.h/cpp         # Bullet fired by ship
    │   ├── AsteroidSurvivorAsteroid.h/cpp           # Asteroid actor with split logic
    │   ├── AsteroidSurvivorAsteroidSpawner.h/cpp    # Wave-based asteroid spawner
    │   └── AsteroidSurvivorHUD.h/cpp                # Canvas-drawn score / lives / wave HUD
    └── AsteroidSurvivorEditor/
        ├── AsteroidSurvivorEditor.Build.cs
        ├── AsteroidSurvivorEditorModule.h           # Editor module interface
        └── AsteroidSurvivorEditorModule.cpp         # Auto-generates .umap files on first launch
```

---

## Requirements

- **Unreal Engine 5.7** (download via the Epic Games Launcher)
- A C++ compiler compatible with UE5 (Visual Studio 2022 on Windows, Xcode on macOS)

---

## Getting Started

1. Clone this repository.
2. Right-click `AsteroidSurvivor.uproject` → **Generate Visual Studio project files**.
3. Open the generated `.sln` in Visual Studio and build the **Development Editor** configuration.
4. Open `AsteroidSurvivor.uproject` in Unreal Editor.
5. The editor module automatically creates the required level maps on first launch.
6. Press **Play** to start the game.

---

## Level Setup

The project includes an **Editor module** (`AsteroidSurvivorEditor`) that **automatically generates** the required `.umap` level files the first time you open the project in Unreal Editor. When the editor starts:

1. If `Content/Maps/GameLevel.umap` does not exist, it is created automatically.
2. If `Content/Maps/MainMenu.umap` does not exist, it is created automatically.
3. The `DefaultGame.ini` is configured so that the editor opens `GameLevel` on startup.

The game mode spawns a default directional light at runtime when the level has no lighting, so the auto-generated empty maps are fully playable out of the box.

> **Note:** If you want a richer environment (sky, fog, ground plane), you can add those actors manually after the map is generated — see [Adding optional environment actors](#adding-optional-environment-actors) below.

### Adding optional environment actors

The auto-generated level provides a functional but minimal play area. For a more polished look, open `Content/Maps/GameLevel` in the editor and add any of the following:

| What to add | How | Why |
|-------------|-----|-----|
| **Sky Atmosphere** | Place Actors panel → **Visual Effects → Sky Atmosphere** | Renders a procedural sky so the background is not black. |
| **Sky Light** | Place Actors panel → **Lights → Sky Light** | Provides ambient/indirect lighting from the sky. Set **Real Time Capture** to `true` so it updates with the atmosphere. |
| **Exponential Height Fog** *(optional)* | Place Actors panel → **Visual Effects → Exponential Height Fog** | Adds atmospheric depth; not strictly necessary for a top-down game but improves the look. |
| **Floor / ground plane** *(optional)* | Place Actors panel → **Basic → Plane** or **Cube**, scale it up, and position it below the play area. | Gives the camera something to see. For a space theme you can skip this and use a dark background or star-field material. |

> **Tip:** You can also create a new level using **File → New Level → Basic** (which includes a directional light, sky atmosphere, sky light, and a floor), save it over `Content/Maps/GameLevel`, and the game mode will use it automatically.

### Assigning the Game Mode

The game mode is configured automatically via `DefaultGame.ini`:

- **GlobalDefaultGameMode** is set to `AsteroidSurvivorGameMode`
- **Default Pawn Class** is `AsteroidSurvivorShip` (set in the C++ game mode constructor)
- **HUD Class** is `AsteroidSurvivorHUD` (set in the C++ game mode constructor)
- **Player Controller** is `AsteroidSurvivorPlayerController` (set in the C++ game mode constructor)

No manual World Settings configuration is required. Simply press **Play** and the game starts.

If you create a Blueprint subclass of `AsteroidSurvivorGameMode`, you can set it as the **GameMode Override** in World Settings to use your customised version instead.

### Making it the default map

The `DefaultGame.ini` already configures `GameLevel` as the **Editor Startup Map**, **Game Default Map**, and **Server Default Map**. No manual Project Settings changes are needed.

After opening the project in Unreal Editor, press **Play** and you should see the ship, asteroids, and the HUD.

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

The game mode now spawns a directional light automatically when the level has no lighting. If you still see a black viewport:

1. Verify that `GlobalDefaultGameMode` in `DefaultGame.ini` is set to `AsteroidSurvivorGameMode`.
2. If using a custom map, ensure the game mode is set in World Settings.

For a richer environment, add a Sky Atmosphere, Sky Light, and Exponential Height Fog to your level manually (see [Adding optional environment actors](#adding-optional-environment-actors)).

### I don't see the ship or asteroids

1. The ship now uses a default **Cone** mesh and the asteroids use engine **Sphere** meshes, so they should be visible out of the box.
2. If you are using Blueprint subclasses, ensure you have assigned a **Static Mesh** to the ship and asteroid Blueprints, or leave the defaults from the C++ base classes.
3. Check that the camera is positioned correctly. The ship creates its own top-down camera via a Spring Arm; make sure the Spring Arm length and camera rotation are sensible (defaults are set in C++).

### The level won't load or the editor opens an untitled map

The `AsteroidSurvivorEditor` module creates `Content/Maps/GameLevel.umap` and `Content/Maps/MainMenu.umap` automatically on first launch. If the maps still don't load:

1. Make sure the C++ code compiled successfully (the editor module must be built).
2. Check the Output Log for messages prefixed with `AsteroidSurvivor:`.
3. You can create the maps manually: **File → New Level → Empty Level**, then **Save As** to `Content/Maps/GameLevel`.

---

## License

This project is provided as-is for educational and portfolio purposes.
