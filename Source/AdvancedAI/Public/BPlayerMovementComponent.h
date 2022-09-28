// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BPlayerMovementComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDAI_API UBPlayerMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPlayerMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const;
	float GetAirResistanceScalar(const FVector& CurrentVelocity) const;
	float GetFrictionResistanceScalar(const FVector& CurrentVelocity) const;

private:

	void UpdateVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);
	void ApplyResistanceToVelocity(float DeltaTime);
	void ApplyInputToVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);


	float ConvertToControlRotationRange(float angle) const;
	void UpdateTransform(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);
	void UpdateRotation(float DeltaTranslationScalar);
	void UpdateLocation(float DeltaTranslationScalar);

	// 속도 관련 계산 ======================================================================================

	/** cm/s */
	UPROPERTY(VisibleAnywhere)
	FVector Velocity;

	/** kg */
	UPROPERTY(EditDefaultsOnly)
	float DefaultMass;

	/**
	 * N = ㎏ × (m/s^2)
	 * cN = ㎏ × (cm/s^2) */
	UPROPERTY(EditDefaultsOnly)
	float DefaultMovingForceScalar;

	UPROPERTY(EditDefaultsOnly)
	float DefaultAccelerationScalar;

	UPROPERTY(EditDefaultsOnly)
	float DragCoefficient;

	UPROPERTY(EditDefaultsOnly)
	float FrictionCoefficient;

	/** cm/s^2 */
	UPROPERTY(VisibleAnywhere)
	float DefaultGravityScalar;

	// 회전 관련 계산 ======================================================================================

	/** cm */
	UPROPERTY(EditDefaultsOnly)
	float MinTurningRadius;

	UPROPERTY(VisibleAnywhere)
	float CurrentYaw;


};
