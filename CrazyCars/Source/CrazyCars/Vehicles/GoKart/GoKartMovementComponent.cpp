// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"
#include "GoKart.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if(AGoKart* goKart = Cast<AGoKart>(GetOwner()))
	{
		m_goKart = goKart;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("movement component is not a child of GoKart!!!"));
	}
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	// ...
}

FVector UGoKartMovementComponent::getRollingResistance()
{
	UWorld* world = GetWorld();
	if (!world)
		return FVector::ZeroVector;

	float accelerationDueToGravity = -world->GetGravityZ() / 100.f; // divided by 100 to get rid of the UE units which is cm
	float normalForce = m_mass * accelerationDueToGravity;
	return -m_velocity.GetSafeNormal() * m_rollingResistanceCoefficient * normalForce;
}

FVector UGoKartMovementComponent::getAirResistance()
{
	float speedSquared = m_velocity.SizeSquared();
	float airResistance = speedSquared * m_dragCoefficient;
	return -m_velocity.GetSafeNormal() * airResistance;
}

void UGoKartMovementComponent::updateLocationFromVelocity(float deltaTime)
{
	FVector translation = m_velocity * 100 * deltaTime; // now in meter

	FHitResult hitResult;
	m_goKart->AddActorWorldOffset(translation, true, &hitResult);
	if (hitResult.bBlockingHit)
	{
		m_velocity = FVector::ZeroVector;
	}
}

void UGoKartMovementComponent::applyRotation(float deltaTime, float steering)
{
	float deltaLocation = FVector::DotProduct(m_goKart->GetActorForwardVector(), m_velocity) * deltaTime;
	float rotationAngle = deltaLocation / m_minimumTurningRadius * steering;
	FQuat rotationDelta = FQuat(m_goKart->GetActorUpVector(), rotationAngle);

	m_velocity = rotationDelta.RotateVector(m_velocity);

	m_goKart->AddActorWorldRotation(rotationDelta);
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& move)
{
	FVector force = m_goKart->GetActorForwardVector() * m_maxDrivingForce * move.Throttle;
	force += getAirResistance();
	force += getRollingResistance();

	FVector acceleration = force / m_mass;
	m_velocity += (acceleration * move.DeltaTime);


	applyRotation(move.DeltaTime, move.Steering);
	updateLocationFromVelocity(move.DeltaTime);
}

FGoKartMove UGoKartMovementComponent::CreateMove(float deltaTime)
{
	FGoKartMove move;
	move.DeltaTime = deltaTime;
	move.Steering = m_steering;
	move.Throttle = m_throttle;
	move.Time = GetWorld()->TimeSeconds;
	return move;
}

void UGoKartMovementComponent::SetThrottle(float throttle)
{
	m_throttle = throttle;
}

void UGoKartMovementComponent::SetSteering(float steering)
{
	m_steering = steering;
}

void UGoKartMovementComponent::SetVelocity(const FVector& velocity)
{
	m_velocity = velocity;
}

