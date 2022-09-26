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

	void UpdateTransform(FVector& CurrentVelocity, float DeltaTime);
	void UpdateRotation(float DeltaTranslationScalar, float DeltaTime);
	void ApplyRotationToVelo
	void UpdateLocation(float DeltaTranslationScalar);

	float GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const;
	float GetAirResistanceScalar(const FVector& CurrentVelocity) const;
	float GetFrictionResistanceScalar() const;

	
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
