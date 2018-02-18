// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

FString ToString(ENetRole role)
{
	switch (role)
	{
	default:
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "Simulated Proxy";
	case ROLE_AutonomousProxy:
		return "Autonomous Proxy";
	case ROLE_Authority:
		return "Authority";
	}
}

void AGoKart::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	FVector force = GetActorForwardVector() * m_maxDrivingForce * m_throttle;
	force += getAirResistance();
	force += getRollingResistance();

	FVector acceleration = force / m_mass;
	m_velocity += (acceleration * deltaTime);

	if(UWorld* const world = GetWorld())
	{
		DrawDebugString(world, FVector::ZeroVector, ToString(Role), this, FColor::White, 0);
	}

	applyRotation(deltaTime);
	updateLocationFromVelocity(deltaTime);
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);

	check(playerInputComponent);

	playerInputComponent->BindAxis("MoveForward", this, &AGoKart::client_moveForward);
	playerInputComponent->BindAxis("MoveRight", this, &AGoKart::client_moveRight);
}

FVector AGoKart::getRollingResistance()
{
	UWorld* world = GetWorld();
	if (!world)
		return FVector::ZeroVector;

	float accelerationDueToGravity = -world->GetGravityZ() / 100.f; // divided by 100 to get rid of the UE units which is cm
	float normalForce = m_mass * accelerationDueToGravity;
	return -m_velocity.GetSafeNormal() * m_rollingResistanceCoefficient * normalForce;
}

FVector AGoKart::getAirResistance()
{
	float speedSquared = m_velocity.SizeSquared();
	float airResistance = speedSquared*m_dragCoefficient;
	return -m_velocity.GetSafeNormal() * airResistance;
}

void AGoKart::applyRotation(float deltaTime)
{
	float deltaLocation = FVector::DotProduct(GetActorForwardVector(), m_velocity) * deltaTime;
	float rotationAngle = deltaLocation / m_minimumTurningRadius * m_steering;
	FQuat rotationDelta = FQuat(GetActorUpVector(), rotationAngle);

	m_velocity = rotationDelta.RotateVector(m_velocity);

	AddActorWorldRotation(rotationDelta);
}

void AGoKart::updateLocationFromVelocity(float deltaTime)
{
	FVector translation = m_velocity * 100 * deltaTime; // now in meter

	FHitResult hitResult;
	AddActorWorldOffset(translation, true, &hitResult);
	if (hitResult.bBlockingHit)
	{
		m_velocity = FVector::ZeroVector;
	}
}

void AGoKart::moveForward(float value)
{
	m_throttle = value;
}

void AGoKart::client_moveForward(float value)
{
	moveForward(value);
	server_moveForward(value);
}

void AGoKart::server_moveForward_Implementation(float value)
{
	moveForward(value);
}

bool AGoKart::server_moveForward_Validate(float value)
{
	return FMath::Abs(value) <= 1.f;
}

void AGoKart::moveRight(float value)
{
	m_steering = value;
}

void AGoKart::client_moveRight(float value)
{
	moveRight(value);
	server_moveRight(value);
}

void AGoKart::server_moveRight_Implementation(float value)
{
	moveRight(value);
}

bool AGoKart::server_moveRight_Validate(float value)
{
	return FMath::Abs(value) <= 1.f;
}
