// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartStructs.h"
#include "GoKart.generated.h"

class UGoKartMovementReplicator;
class UGoKartMovementComponent;
UCLASS()
class CRAZYCARS_API AGoKart : public APawn
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(VisibleAnywhere)
	UGoKartMovementComponent* m_movementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UGoKartMovementReplicator* m_movementReplicator = nullptr;

public:
	AGoKart();
	virtual void Tick(float deltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;
	FORCEINLINE_DEBUGGABLE UGoKartMovementComponent* GetGoKartMovementComponent() const { return m_movementComponent; }

protected:
	virtual void BeginPlay() override;

private:
	void moveForward(float value);
	void moveRight(float value);
};
