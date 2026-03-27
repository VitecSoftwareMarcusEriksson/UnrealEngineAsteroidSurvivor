// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnemyShipBase.h"
#include "ZigzagEnemyShip.generated.h"

/**
 * Zigzag enemy ship – moves toward the player in a weaving pattern.
 *
 * Behaviour:
 * - Advances toward the player at moderate speed.
 * - Oscillates laterally (perpendicular to the player direction)
 *   creating a zigzag flight path that makes it harder to hit.
 * - Does NOT fire projectiles.
 * - Slightly more health than a standard ship.
 *
 * The zigzag amplitude and frequency are configurable.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AZigzagEnemyShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	AZigzagEnemyShip();

	/** Set the zigzag phase so formation ships oscillate in sync. */
	void SetZigzagPhase(float Phase) { ZigzagTimer = Phase; }

protected:
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip) override;

	/** Lateral oscillation amplitude (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Zigzag")
	float ZigzagAmplitude = 300.0f;

	/** Oscillation frequency (cycles per second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Zigzag")
	float ZigzagFrequency = 1.5f;

	/** Turn rate for tracking the player (degrees/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	float TurnRate = 60.0f;

private:
	/** Accumulated time for the sine wave oscillation. */
	float ZigzagTimer = 0.0f;
};
