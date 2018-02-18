// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

//HINT: you can't change the name of the parameter! otherwise the DOREPLIFETIME macro won't work because it uses OutLifetimeProp!
void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, m_replicatedServerState);
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

	//we are the player/client
	if(Role == ROLE_AutonomousProxy)
	{
		const FGoKartMove move = createMove(deltaTime);
		simulateMove(move);
		m_unacknowledgedMoves.Add(move);
		server_sendMove(move);
	}

	//we are the server and in control of the pawn
	if(HasAuthority() && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		const FGoKartMove move = createMove(deltaTime);
		server_sendMove(move);
	}

	//we are another player/client in the actual player's/client's game
	if(Role == ROLE_SimulatedProxy)
	{
		simulateMove(m_replicatedServerState.LastMove);
	}

	if (UWorld* const world = GetWorld())
	{
		DrawDebugString(world, FVector::ZeroVector, ToString(Role), this, FColor::White, 0);
	}
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);

	check(playerInputComponent);

	playerInputComponent->BindAxis("MoveForward", this, &AGoKart::moveForward);
	playerInputComponent->BindAxis("MoveRight", this, &AGoKart::moveRight);
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

void AGoKart::applyRotation(float deltaTime, float steering)
{
	float deltaLocation = FVector::DotProduct(GetActorForwardVector(), m_velocity) * deltaTime;
	float rotationAngle = deltaLocation / m_minimumTurningRadius * steering;
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

void AGoKart::moveRight(float value)
{
	m_steering = value;
}

void AGoKart::simulateMove(const FGoKartMove& move)
{
	FVector force = GetActorForwardVector() * m_maxDrivingForce * move.Throttle;
	force += getAirResistance();
	force += getRollingResistance();

	FVector acceleration = force / m_mass;
	m_velocity += (acceleration * move.DeltaTime);


	applyRotation(move.DeltaTime, move.Steering);
	updateLocationFromVelocity(move.DeltaTime);
}

FGoKartMove AGoKart::createMove(float deltaTime)
{
	FGoKartMove move;
	move.DeltaTime = deltaTime;
	move.Steering = m_steering;
	move.Throttle = m_throttle;
	move.Time = GetWorld()->TimeSeconds;
	return move;
}

void AGoKart::clearUnacknowledgedMoves(const FGoKartMove& lastMove)
{
	TArray<FGoKartMove> newMoves;
	for(int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		const FGoKartMove move = m_unacknowledgedMoves[i];
		if(move.Time > lastMove.Time)
		{
			newMoves.Add(move);
		}
	}
	m_unacknowledgedMoves = newMoves;
}

void AGoKart::server_sendMove_Implementation(FGoKartMove move)
{
	simulateMove(move);

	m_replicatedServerState.LastMove = move;
	m_replicatedServerState.Transform = GetActorTransform();
	m_replicatedServerState.Velocity = m_velocity;
}

bool AGoKart::server_sendMove_Validate(FGoKartMove move)
{
	return FMath::Abs(move.Throttle) <= 1.f && FMath::Abs(move.Steering) <= 1.f;
}

void AGoKart::onRep_serverState()
{
	SetActorTransform(m_replicatedServerState.Transform);
	m_velocity = m_replicatedServerState.Velocity;
	clearUnacknowledgedMoves(m_replicatedServerState.LastMove);

	for(int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		simulateMove(m_unacknowledgedMoves[i]);
	}
}