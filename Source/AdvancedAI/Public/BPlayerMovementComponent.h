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
		SharedWorldTime = InputSharedWorldTime;
		DeltaTime = InputDeltaTime;
		ForwardMovementFactor = InputForwardMovementFactor;
		RightMovementFactor = InputRightMovementFactor;
	}

	/** 입력 값 검증을 통한 치트 방지 */
	bool IsValid() const
	{
		return (1 >= FMath::Abs(ForwardMovementFactor)) 
			&& (1 >= FMath::Abs(RightMovementFactor));
	}
};


USTRUCT()
struct FPlayerMovementState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Tranform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	float LastMovementObjectSharedWorldTime;

	/** 서버에서 이 패킷을 보낸 시각, 1/2 RTT를 계산하여 조금 더 정확한 시뮬레이션을 하기 위해 사용된다. */
	UPROPERTY()
	float DepartureTime;
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	void SetMaxVelocityFactor(const float NewMaxVelocityFactor);

	UFUNCTION(BlueprintCallable)
	FVector GetVelocity() const;

	UFUNCTION(BlueprintCallable)
	float GetCurrentYaw() const;


	// Begin : Replication 관련 코드 =============================================================================================
private:

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMove(const FPlayerMovementObject& MovementObject);

	// bool CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const;

	void UpdateMovementState(const FPlayerMovementObject& MovementObject);

	/** 서버에서 처리된 MovementObject를 삭제하는 함수 */
	void RemoveProcessedMovementObjects(float LastMovementObjectSharedWorldTime);


	UFUNCTION()
	void OnRep_MovementState();

	/** 서버에서 리플리케이션되는 이동 정보 */
	UPROPERTY(ReplicatedUsing = OnRep_MovementState)
	FPlayerMovementState MovementState;

	/** 
	 * 폰을 컨트롤하는 곳에서 정확한 시뮬레이션을 하기 위해 플레이어의 최신 이동 입력들을 저장하고 있다.
	 * 서버에서 처리되는대로 삭제된다. */
	TArray<FPlayerMovementObject> PendingMovementObjects;

	/** DeltaTime을 사용한 치트 방지 */
	float ClientSimulatedTime;

	// End : Replication 관련 코드 =============================================================================================
	
	// Begin : Transform, Physics 관련 코드 ======================================================================================
private:

	/// 이동 관련

	void RefreshMovementVariable();

	float GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const;

	float GetResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetAirResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetFrictionResistanceScalar() const;

	void SimulateMovementObject(const FPlayerMovementObject& MovementObject);
	void SetVelocity(const FVector& NewVelocity);
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


	/// 회전 관련

	/** cm */
	UPROPERTY(EditDefaultsOnly)
	float MinTurningRadius;

	UPROPERTY(VisibleAnywhere)
	float CurrentYaw;

	// End : Transform, Physics 관련 코드 ======================================================================================
};
