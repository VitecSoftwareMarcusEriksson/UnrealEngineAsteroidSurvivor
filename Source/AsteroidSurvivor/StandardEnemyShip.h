// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnemyShipBase.h"
#include "StandardEnemyShip.generated.h"

/**
 * Standard enemy ship – the most common enemy type.
 *
 * Behaviour:
 * - Moves slowly toward the player ship in a straight line.
 * - Does NOT fire projectiles.
 * - Low health, low score value.
 * - Drops a small amount of scrap on destruction.
 *
 * The ship adjusts its heading each frame to track the player,
 * with a turn rate limit so it cannot instantly snap to the player's
 * position (allowing skilled players to dodge).
 */
UCLASS()
class ASTEROIDSURVIVOR_API AStandardEnemyShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	AStandardEnemyShip();

protected:
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip) override;

	/** Maximum turn rate toward the player (degrees/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	float TurnRate = 45.0f;
};
