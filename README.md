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
5. Create or open the gameplay level, assign `BP_AsteroidSurvivorGameMode` as the game mode.
6. Press **Play** to start the game.

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

## License

This project is provided as-is for educational and portfolio purposes.