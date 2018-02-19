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

	if (!m_goKartMovementComponent)
		return;

	const FGoKartMove lastMove = m_goKartMovementComponent->GetLastMove();

	//we are the player/client
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		m_unacknowledgedMoves.Add(lastMove);
		server_sendMove(lastMove);
	}

	//we are the server and in control of the pawn
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		updateServerState(lastMove);
	}

	//we are another player/client in the actual player's/client's game
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		client_tick(deltaTime);
	}
}

void UGoKartMovementReplicator::client_tick(float deltaTime)
{
	m_client_timeSinceUpdate += deltaTime;
	
	if (m_client_timeBetweenLastUpdate < KINDA_SMALL_NUMBER)
		return;

	const float lerpRatio = m_client_timeSinceUpdate / m_client_timeBetweenLastUpdate;
	const FTransform startTransform = m_client_startTransform;
	const FTransform targetTransform = m_replicatedServerState.Transform;

	FTransform newTransform = m_client_startTransform;
	newTransform.SetLocation(FMath::LerpStable(startTransform.GetLocation(), targetTransform.GetLocation(), lerpRatio));
	newTransform.SetRotation(FQuat::Slerp(startTransform.GetRotation(), targetTransform.GetRotation(), lerpRatio));
	GetOwner()->SetActorTransform(newTransform);
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

void UGoKartMovementReplicator::updateServerState(const FGoKartMove& move)
{
	m_replicatedServerState.LastMove = move;
	m_replicatedServerState.Transform = GetOwner()->GetActorTransform();
	m_replicatedServerState.Velocity = m_goKartMovementComponent->GetVelocity();
}

void UGoKartMovementReplicator::server_sendMove_Implementation(FGoKartMove move)
{
	if (!m_goKartMovementComponent)
		return;

	m_goKartMovementComponent->SimulateMove(move);

	updateServerState(move);
}

bool UGoKartMovementReplicator::server_sendMove_Validate(FGoKartMove move)
{
	return FMath::Abs(move.Throttle) <= 1.f && FMath::Abs(move.Steering) <= 1.f;
}

void UGoKartMovementReplicator::onRep_serverState()
{
	switch(GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		autonomousProxy_onRep_serverState();
		break;
	case ROLE_SimulatedProxy:
		simulatedProxy_onRep_serverState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicator::autonomousProxy_onRep_serverState()
{
	if (!m_goKartMovementComponent)
		return;

	GetOwner()->SetActorTransform(m_replicatedServerState.Transform);
	m_goKartMovementComponent->SetVelocity(m_replicatedServerState.Velocity);
	clearUnacknowledgedMoves(m_replicatedServerState.LastMove);

	for (int32 i = 0; i < m_unacknowledgedMoves.Num(); ++i)
	{
		m_goKartMovementComponent->SimulateMove(m_unacknowledgedMoves[i]);
	}
}

void UGoKartMovementReplicator::simulatedProxy_onRep_serverState()
{
	m_client_timeBetweenLastUpdate = m_client_timeSinceUpdate;
	m_client_timeSinceUpdate = 0;
	m_client_startTransform = GetOwner()->GetActorTransform();
}
