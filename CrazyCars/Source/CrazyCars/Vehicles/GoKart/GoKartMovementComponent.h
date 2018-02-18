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
	// Sets default values for this component's properties
	UGoKartMovementComponent();
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	void SimulateMove(const FGoKartMove& move);
	FGoKartMove CreateMove(float deltaTime);
	void SetThrottle(float throttle);
	void SetSteering(float steering);
	void SetVelocity(const FVector& velocity);
	FORCEINLINE_DEBUGGABLE FVector GetVelocity() const { return m_velocity; };

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	FVector getRollingResistance();
	FVector getAirResistance();
	void updateLocationFromVelocity(float deltaTime);
	void applyRotation(float deltaTime, float steering);


private:
	AGoKart* m_goKart = nullptr;

	FVector m_velocity = FVector::ZeroVector;
	float m_throttle = 0;
	float m_steering = 0;

};
