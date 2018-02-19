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
	void updateServerState(const FGoKartMove& move);
	void client_tick(float deltaTime);

	UFUNCTION(Server, Reliable, WithValidation)
	void server_sendMove(FGoKartMove move);

	UFUNCTION()
	void onRep_serverState();
	void autonomousProxy_onRep_serverState();
	void simulatedProxy_onRep_serverState();
		
private:
	TArray<FGoKartMove> m_unacknowledgedMoves;

	UGoKartMovementComponent* m_goKartMovementComponent = nullptr;

	float m_client_timeSinceUpdate = 0;
	float m_client_timeBetweenLastUpdate = 0;
	FTransform m_client_startTransform;
	FVector m_client_startVelocity;
};
