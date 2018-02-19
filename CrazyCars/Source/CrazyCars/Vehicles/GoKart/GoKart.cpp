// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateMovement = false;

	m_movementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(FName("Movement Component"));
	m_movementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>(FName("Movement Replicator"));
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

void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		//creates a lag on the client
		NetUpdateFrequency = 1;
	}
}

void AGoKart::moveForward(float value)
{
	m_movementComponent->SetThrottle(value);
}

void AGoKart::moveRight(float value)
{
	m_movementComponent->SetSteering(value);
}
