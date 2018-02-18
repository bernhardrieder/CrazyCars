// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "GoKartMovementComponent.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	m_movementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(FName("Movement Component"));
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
		const FGoKartMove move = m_movementComponent->CreateMove(deltaTime);
		m_movementComponent->SimulateMove(move);
		m_unacknowledgedMoves.Add(move);
		server_sendMove(move);
	}

	//we are the server and in control of the pawn
	if(HasAuthority() && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		const FGoKartMove move = m_movementComponent->CreateMove(deltaTime);
		server_sendMove(move);
	}

	//we are another player/client in the actual player's/client's game
	if(Role == ROLE_SimulatedProxy)
	{
		m_movementComponent->SimulateMove(m_replicatedServerState.LastMove);
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

void AGoKart::moveForward(float value)
{
	m_movementComponent->SetThrottle(value);
}

void AGoKart::moveRight(float value)
{
	m_movementComponent->SetSteering(value);
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
	m_movementComponent->SimulateMove(move);

	m_replicatedServerState.LastMove = move;
	m_replicatedServerState.Transform = GetActorTransform();
	m_replicatedServerState.Velocity = m_movementComponent->GetVelocity();
}

bool AGoKart::server_sendMove_Validate(FGoKartMove move)
{
	return FMath::Abs(move.Throttle) <= 1.f && FMath::Abs(move.Steering) <= 1.f;
}

void AGoKart::onRep_serverState()
{
	SetActorTransform(m_replicatedServerState.Transform);
	m_movementComponent->SetVelocity(m_replicatedServerState.Velocity);
	clearUnacknowledgedMoves(m_replicatedServerState.LastMove);

	for(int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		m_movementComponent->SimulateMove(m_unacknowledgedMoves[i]);
	}
}