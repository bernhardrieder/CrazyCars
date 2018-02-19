// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartStructs.h"
#include "GoKartMovementReplicator.generated.h"

class UGoKartMovementComponent;
class AGoKart;

struct FHermiteCubicSpline
{
	FVector StartLocation;
	FVector StartDerivative;
	FVector TargetLocation;
	FVector TargetDerivative;

	FVector InterpolateLocation(float lerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, lerpRatio);
	}
	FVector InterpolateDerivative(float lerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, lerpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CRAZYCARS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(ReplicatedUsing = onRep_serverState)
	FGoKartState m_replicatedServerState;

public:	
	UGoKartMovementReplicator();
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* meshOffsetRoot) { m_meshOffsetRoot = meshOffsetRoot; }

protected:
	virtual void BeginPlay() override;

private:
	void clearUnacknowledgedMoves(const FGoKartMove& lastMove);
	void updateServerState(const FGoKartMove& move);
	void client_tick(float deltaTime);
	FHermiteCubicSpline createSpline() const;
	float getVelocityToDerivate() const;
	void interpolateLocation(const FHermiteCubicSpline& spline, float lerpRatio) const;
	void interpolateRotation(float lerpRatio) const;
	void interpolateVelocity(const FHermiteCubicSpline& spline, float lerpRatio) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void server_sendMove(FGoKartMove move);

	UFUNCTION()
	void onRep_serverState();
	void autonomousProxy_onRep_serverState();
	void simulatedProxy_onRep_serverState();
		
private:
	TArray<FGoKartMove> m_unacknowledgedMoves;

	UGoKartMovementComponent* m_goKartMovementComponent = nullptr;
	USceneComponent* m_meshOffsetRoot = nullptr;

	float m_client_timeSinceUpdate = 0;
	float m_client_timeBetweenLastUpdate = 0;
	FTransform m_client_startTransform;
	FVector m_client_startVelocity;
	float m_client_simulatedTime = 0;
};
