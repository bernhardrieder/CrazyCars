// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"
#include "GoKart.h"
#include "Engine/World.h"

UGoKartMovementComponent::UGoKartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if(AGoKart* goKart = Cast<AGoKart>(GetOwner()))
	{
		m_goKart = goKart;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("movement component is not a child of GoKart!!!"));
	}
	
}

void UGoKartMovementComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy || m_goKart->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		m_lastMove = createMove(deltaTime);
		SimulateMove(m_lastMove);
	}
}

FVector UGoKartMovementComponent::getRollingResistance() const
{
	UWorld* world = GetWorld();
	if (!world)
		return FVector::ZeroVector;

	const float accelerationDueToGravity = -world->GetGravityZ() / 100.f; // divided by 100 to get rid of the UE units which is cm
	const float normalForce = m_mass * accelerationDueToGravity;
	return -m_velocity.GetSafeNormal() * m_rollingResistanceCoefficient * normalForce;
}

FVector UGoKartMovementComponent::getAirResistance() const
{
	const float speedSquared = m_velocity.SizeSquared();
	const float airResistance = speedSquared * m_dragCoefficient;
	return -m_velocity.GetSafeNormal() * airResistance;
}

void UGoKartMovementComponent::updateLocationFromVelocity(float deltaTime)
{
	if (!m_goKart)
		return;

	const FVector translation = m_velocity * 100 * deltaTime; // now in meter

	FHitResult hitResult;
	m_goKart->AddActorWorldOffset(translation, true, &hitResult);
	if (hitResult.bBlockingHit)
	{
		m_velocity = FVector::ZeroVector;
	}
}

void UGoKartMovementComponent::applyRotation(float deltaTime, float steering)
{
	if (!m_goKart)
		return;

	const float deltaLocation = FVector::DotProduct(m_goKart->GetActorForwardVector(), m_velocity) * deltaTime;
	const float rotationAngle = deltaLocation / m_minimumTurningRadius * steering;
	FQuat rotationDelta = FQuat(m_goKart->GetActorUpVector(), rotationAngle);

	m_velocity = rotationDelta.RotateVector(m_velocity);

	m_goKart->AddActorWorldRotation(rotationDelta);
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& move)
{
	if (!m_goKart)
		return;

	FVector force = m_goKart->GetActorForwardVector() * m_maxDrivingForce * move.Throttle;
	force += getAirResistance();
	force += getRollingResistance();

	const FVector acceleration = force / m_mass;
	m_velocity += (acceleration * move.DeltaTime);


	applyRotation(move.DeltaTime, move.Steering);
	updateLocationFromVelocity(move.DeltaTime);
}

FGoKartMove UGoKartMovementComponent::createMove(float deltaTime) const
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

