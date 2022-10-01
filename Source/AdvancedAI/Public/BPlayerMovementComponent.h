// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BPlayerMovementComponent.generated.h"

USTRUCT()
struct FPlayerMovementObject
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float SharedWorldTime;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float ForwardMovementFactor;

	UPROPERTY()
	float RightMovementFactor;

	void Set(float InputSharedWorldTime, float InputDeltaTime, float InputForwardMovementFactor, float InputRightMovementFactor)
	{
		this->SharedWorldTime = InputSharedWorldTime;
		this->DeltaTime = InputDeltaTime;
		this->ForwardMovementFactor = InputForwardMovementFactor;
		this->RightMovementFactor = InputRightMovementFactor;
	}

	bool IsValid() const
	{
		return (1 >= FMath::Abs(ForwardMovementFactor)) 
			&& (1 >= FMath::Abs(RightMovementFactor));
	}
};
/*
USTRUCT()
struct FPlayerMovementState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Tranform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FPlayerMovementObject LastMovementObject;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
};
*/

UCLASS(/*ClassGroup = (Custom), meta = (BlueprintSpawnableComponent)*/)
class ADVANCEDAI_API UBPlayerMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPlayerMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMaxVelocityFactor(const float NewMaxVelocityFactor);

	UFUNCTION(BlueprintCallable)
	FVector GetPlayerVelocity() const;

	UFUNCTION(BlueprintCallable)
	float GetCurrentYaw() const;



private:
	// Begin : Replication 관련 코드 =============================================================================================

	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerMove(FPlayerMovementObject MovementObject);



	// bool CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const;

	
	void SimulateMovementObject(const FPlayerMovementObject& MovementObject);




	/** 플레이어의 이동 입력 */
	FPlayerMovementObject LastMovementObject;

	// End : Replication 관련 코드 =============================================================================================
	
	// Begin : Transform, Physics 관련 코드 ======================================================================================

	void RefreshMovementVariable();

	float GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const;

	float GetResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetAirResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetFrictionResistanceScalar() const;

	void UpdateVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);
	void ApplyResistanceToVelocity(float DeltaTime);
	void ApplyInputToVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);

	float ConvertToControlRotationRange(float angle) const;
	void UpdateTransform(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime);
	void UpdateRotation(float DeltaTranslationScalar);
	void UpdateLocation(float DeltaTranslationScalar);

	UPROPERTY(EditDefaultsOnly)
	float MaxVelocityScalar;

	UPROPERTY(VisibleAnywhere)
	float CurrentMaxVelocityScalar;

	UPROPERTY(VisibleAnywhere)
	float CurrentMaxVelocityFactor;

	/** cm/s */
	UPROPERTY(VisibleAnywhere)
	FVector Velocity;

	/** kg */
	UPROPERTY(EditDefaultsOnly)
	float DefaultMass;

	/**
	 * N = ㎏ × (m/s^2)
	 * cN = ㎏ × (cm/s^2) */
	UPROPERTY(VisibleAnywhere)
	float MovementForceScalar;

	UPROPERTY(VisibleAnywhere)
	float AccelerationScalar;

	UPROPERTY(EditDefaultsOnly)
	float DragCoefficient;

	UPROPERTY(EditDefaultsOnly)
	float FrictionCoefficient;

	/** cm/s^2 */
	UPROPERTY(VisibleAnywhere)
	float DefaultGravityScalar;

	/** 회전 관련 */

	/** cm */
	UPROPERTY(EditDefaultsOnly)
	float MinTurningRadius;

	UPROPERTY(VisibleAnywhere)
	float CurrentYaw;

	// End : Transform, Physics 관련 코드 ======================================================================================

};
