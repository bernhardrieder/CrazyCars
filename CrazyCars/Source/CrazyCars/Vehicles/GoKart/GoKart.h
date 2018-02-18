// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float Steering;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;
};

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGoKartMove LastMove;
	
	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FTransform Transform;
};

UCLASS()
class CRAZYCARS_API AGoKart : public APawn
{
	GENERATED_BODY()
	
	UPROPERTY(ReplicatedUsing = onRep_serverState)
	FGoKartState m_replicatedServerState;

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
	AGoKart();

public:	
	virtual void Tick(float deltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;

private:
	FVector getRollingResistance();
	FVector getAirResistance();
	void updateLocationFromVelocity(float deltaTime);
	void applyRotation(float deltaTime, float steering);

	void moveForward(float value);
	void moveRight(float value);
	void simulateMove(const FGoKartMove& move);

	UFUNCTION(Server, Reliable, WithValidation)
	void server_sendMove(FGoKartMove move);

	UFUNCTION()
	void onRep_serverState();

private:
	FVector m_velocity = FVector::ZeroVector;
	float m_throttle = 0;
	float m_steering = 0;
};
