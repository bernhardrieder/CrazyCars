// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartStructs.h"
#include "GoKartMovementComponent.generated.h"

class AGoKart;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRAZYCARS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

	// the mass of the car (kg)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Mass"))
	float m_mass = 1000;

	// the force applied to the car when the throttle is fully down (N)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Max Driving Force"))
	float m_maxDrivingForce = 10000;

	// minimum radius of the car turning circle at full lock (m)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Minimum Turning Radius"))
	float m_minimumTurningRadius = 10;

	// higher means more drag/ air resistance
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Drag Coefficient"))
	float m_dragCoefficient = 16.0f;

	// higher means more rolling resistance
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Rolling Resistance Coefficient"))
	float m_rollingResistanceCoefficient = 0.015f;

public:	
	UGoKartMovementComponent();
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	void SimulateMove(const FGoKartMove& move);
	void SetThrottle(float throttle);
	void SetSteering(float steering);
	void SetVelocity(const FVector& velocity);
	FORCEINLINE_DEBUGGABLE FVector GetVelocity() const { return m_velocity; };
	FORCEINLINE_DEBUGGABLE FGoKartMove GetLastMove() const { return m_lastMove; };

protected:
	virtual void BeginPlay() override;

private:
	FVector getRollingResistance() const;
	FVector getAirResistance() const;
	void updateLocationFromVelocity(float deltaTime);
	void applyRotation(float deltaTime, float steering);

	FGoKartMove createMove(float deltaTime) const;

private:
	AGoKart* m_goKart = nullptr;
	FGoKartMove m_lastMove;
	FVector m_velocity = FVector::ZeroVector;
	float m_throttle = 0;
	float m_steering = 0;

};
