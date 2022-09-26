// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BPlayerMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDAI_API UBPlayerMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBPlayerMovementComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	float GetAirResistance();
	float GetFrictionResistance(const FVector& currentVelocity);


	/* cm/s */
	UPROPERTY(VisibleAnywhere)
	FVector Velocity;

	/* kg */
	UPROPERTY(EditDefaultsOnly)
	float DefaultMass;

	// N = §¸ ¡¿ (m/s^2)
	/* cN = §¸ ¡¿ (cm/s^2) */
	UPROPERTY(EditDefaultsOnly)
	float DefaultMovingForce;

	/* cm */
	UPROPERTY(EditDefaultsOnly)
	float MinTurningRadius;

	UPROPERTY(EditDefaultsOnly)
	float DragCoefficient;

	UPROPERTY(EditDefaultsOnly)
	float FrictionCoefficient;

	/* cm/s^2 */
	UPROPERTY(VisibleAnywhere)
	float DefaultGravity;
		
};
