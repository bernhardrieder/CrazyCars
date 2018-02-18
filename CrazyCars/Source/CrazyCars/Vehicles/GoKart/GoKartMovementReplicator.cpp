// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "GoKartStructs.h"
#include "GoKart.h"
#include "GoKartMovementComponent.h"
#include "UnrealNetwork.h"

UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

//HINT: you can't change the name of the parameter! otherwise the DOREPLIFETIME macro won't work because it uses OutLifetimeProp!
void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicator, m_replicatedServerState);
}

void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	if (AGoKart* goKart = Cast<AGoKart>(GetOwner()))
	{
		m_goKart = goKart;
		m_goKartMovementComponent = goKart->GetGoKartMovementComponent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("movement component is not a child of GoKart!!!"));
	}
}

void UGoKartMovementReplicator::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (!m_goKart)
		return;

	//we are the player/client
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		const FGoKartMove move = m_goKartMovementComponent->CreateMove(deltaTime);
		m_goKartMovementComponent->SimulateMove(move);
		m_unacknowledgedMoves.Add(move);
		server_sendMove(move);
	}

	//we are the server and in control of the pawn
	if (m_goKart->HasAuthority() && m_goKart->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		const FGoKartMove move = m_goKartMovementComponent->CreateMove(deltaTime);
		server_sendMove(move);
	}

	//we are another player/client in the actual player's/client's game
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		m_goKartMovementComponent->SimulateMove(m_replicatedServerState.LastMove);
	}
}

void UGoKartMovementReplicator::clearUnacknowledgedMoves(const FGoKartMove& lastMove)
{
	TArray<FGoKartMove> newMoves;
	for (int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		const FGoKartMove move = m_unacknowledgedMoves[i];
		if (move.Time > lastMove.Time)
		{
			newMoves.Add(move);
		}
	}
	m_unacknowledgedMoves = newMoves;
}

void UGoKartMovementReplicator::server_sendMove_Implementation(FGoKartMove move)
{
	if (!m_goKart)
		return;

	m_goKartMovementComponent->SimulateMove(move);

	m_replicatedServerState.LastMove = move;
	m_replicatedServerState.Transform = m_goKart->GetActorTransform();
	m_replicatedServerState.Velocity = m_goKartMovementComponent->GetVelocity();
}

bool UGoKartMovementReplicator::server_sendMove_Validate(FGoKartMove move)
{
	return FMath::Abs(move.Throttle) <= 1.f && FMath::Abs(move.Steering) <= 1.f;
}

void UGoKartMovementReplicator::onRep_serverState()
{
	if (!m_goKart)
		return;

	m_goKart->SetActorTransform(m_replicatedServerState.Transform);
	m_goKartMovementComponent->SetVelocity(m_replicatedServerState.Velocity);
	clearUnacknowledgedMoves(m_replicatedServerState.LastMove);

	for (int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		m_goKartMovementComponent->SimulateMove(m_unacknowledgedMoves[i]);
	}
}