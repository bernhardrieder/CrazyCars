// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartStructs.h"
#include "GoKartMovementReplicator.generated.h"

class UGoKartMovementComponent;
class AGoKart;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CRAZYCARS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(ReplicatedUsing = onRep_serverState)
	FGoKartState m_replicatedServerState;

public:	
	UGoKartMovementReplicator();
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void clearUnacknowledgedMoves(const FGoKartMove& lastMove);

	UFUNCTION(Server, Reliable, WithValidation)
	void server_sendMove(FGoKartMove move);

	UFUNCTION()
	void onRep_serverState();
		
private:
	TArray<FGoKartMove> m_unacknowledgedMoves;

	AGoKart* m_goKart = nullptr;
	UGoKartMovementComponent* m_goKartMovementComponent = nullptr;
};
