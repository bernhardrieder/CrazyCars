// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGoKart::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	FVector force = GetActorForwardVector() * m_maxDrivingForce * m_throttle;
	FVector acceleration = force / m_mass;
	m_velocity += (acceleration * deltaTime);

	applyRotation(deltaTime);
	updateLocationFromVelocity(deltaTime);
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);

	check(playerInputComponent);

	playerInputComponent->BindAxis("MoveForward", this, &AGoKart::moveForward);
	playerInputComponent->BindAxis("MoveRight", this, &AGoKart::moveRight);
}

void AGoKart::applyRotation(float deltaTime)
{
	float rotationAngle = m_maxSteeringDegreesPerSeconds * deltaTime * m_steering;
	FQuat rotationDelta = FQuat(GetActorUpVector(), FMath::DegreesToRadians(rotationAngle));

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

void AGoKart::moveRight(float value)
{
	m_steering = value;
}