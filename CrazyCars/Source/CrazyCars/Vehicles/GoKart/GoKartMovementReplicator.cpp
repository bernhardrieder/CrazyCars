// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "GoKartStructs.h"
#include "GoKart.h"
#include "GoKartMovementComponent.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"

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

	if (!m_goKartMovementComponent || m_client_timeBetweenLastUpdate < KINDA_SMALL_NUMBER)
		return;
	
	const float lerpRatio = m_client_timeSinceUpdate / m_client_timeBetweenLastUpdate;
	const FHermiteCubicSpline spline = createSpline();
	interpolateLocation(spline, lerpRatio);
	interpolateVelocity(spline, lerpRatio);
	interpolateRotation(lerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicator::createSpline() const
{
	FHermiteCubicSpline spline;
	spline.StartLocation = m_client_startTransform.GetLocation();
	spline.TargetLocation = m_replicatedServerState.Transform.GetLocation();
	spline.StartDerivative = m_client_startVelocity * getVelocityToDerivate();
	spline.TargetDerivative = m_replicatedServerState.Velocity * getVelocityToDerivate();
	return spline;
}

float UGoKartMovementReplicator::getVelocityToDerivate() const
{
	//velocity is in m/s BUT position is in cm! so we need to multiply by 100 to get cm
	return m_client_timeBetweenLastUpdate * 100; 
}

void UGoKartMovementReplicator::interpolateLocation(const FHermiteCubicSpline& spline, float lerpRatio) const
{
	if(m_meshOffsetRoot)
	{
		m_meshOffsetRoot->SetWorldLocation(spline.InterpolateLocation(lerpRatio));
	}
}

void UGoKartMovementReplicator::interpolateRotation(float lerpRatio) const
{
	if (m_meshOffsetRoot)
	{
		m_meshOffsetRoot->SetWorldRotation(FQuat::Slerp(m_client_startTransform.GetRotation(), m_replicatedServerState.Transform.GetRotation(), lerpRatio));
	}
}

void UGoKartMovementReplicator::interpolateVelocity(const FHermiteCubicSpline& spline, float lerpRatio) const
{
	const FVector newDerivative = spline.InterpolateDerivative(lerpRatio);
	const FVector newVelocity = newDerivative / getVelocityToDerivate();
	m_goKartMovementComponent->SetVelocity(newVelocity);
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

	m_client_simulatedTime += move.DeltaTime;
	m_goKartMovementComponent->SimulateMove(move);

	updateServerState(move);
}

bool UGoKartMovementReplicator::server_sendMove_Validate(FGoKartMove move)
{
	const float proposedTime = m_client_simulatedTime + move.DeltaTime;
	if(UWorld* const world = GetWorld())
	{
		const bool isClientNotRunningAhead = proposedTime < GetWorld()->TimeSeconds;
		if(!isClientNotRunningAhead)
		{
			UE_LOG(LogTemp, Error, TEXT("Client is running too fast!"));
			return false;
		}
	}
	if(!move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move!"));
		return false;
	}

	return true;
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
	if (!m_goKartMovementComponent)
		return;

	m_client_timeBetweenLastUpdate = m_client_timeSinceUpdate;
	m_client_timeSinceUpdate = 0;

	if(m_meshOffsetRoot)
	{
		m_client_startTransform.SetLocation(m_meshOffsetRoot->GetComponentLocation());
		m_client_startTransform.SetRotation(m_meshOffsetRoot->GetComponentRotation().Quaternion());
	}
	m_client_startVelocity = m_goKartMovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(m_replicatedServerState.Transform);
}
