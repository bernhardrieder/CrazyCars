// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartStructs.h"
#include "GoKart.generated.h"

class UGoKartMovementComponent;
UCLASS()
class CRAZYCARS_API AGoKart : public APawn
{
	GENERATED_BODY()
	
	UPROPERTY(ReplicatedUsing = onRep_serverState)
	FGoKartState m_replicatedServerState;

	UPROPERTY(EditAnywhere)
	UGoKartMovementComponent* m_movementComponent = nullptr;

public:
	AGoKart();
	virtual void Tick(float deltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;

private:
	void moveForward(float value);
	void moveRight(float value);
	void clearUnacknowledgedMoves(const FGoKartMove& lastMove);

	UFUNCTION(Server, Reliable, WithValidation)
	void server_sendMove(FGoKartMove move);

	UFUNCTION()
	void onRep_serverState();

private:

	TArray<FGoKartMove> m_unacknowledgedMoves;
};
