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

	UPROPERTY()
	FRotator ControlRotation;

	void Set(float InputSharedWorldTime, float InputDeltaTime, 
		float InputForwardMovementFactor, float InputRightMovementFactor, const FRotator& InputControlRotation)
	{
		SharedWorldTime = InputSharedWorldTime;
		DeltaTime = InputDeltaTime;
		ForwardMovementFactor = InputForwardMovementFactor;
		RightMovementFactor = InputRightMovementFactor;
		ControlRotation = InputControlRotation;
	}

	/** 입력 값 검증을 통한 치트 방지 */
	bool IsValid() const
	{
		return (1 >= FMath::Abs(ForwardMovementFactor)) 
			&& (1 >= FMath::Abs(RightMovementFactor));
	}

	void Print() const
	{
		B_LOG_DEV("SharedWorldTime : %.1f", SharedWorldTime);
		B_LOG_DEV("DeltaTime : %.1f", DeltaTime);
		B_LOG_DEV("ForwardMovementFactor : %.1f", ForwardMovementFactor);
		B_LOG_DEV("RightMovementFactor : %.1f", RightMovementFactor);
		B_LOG_DEV("ControlRotation : %.1f, %.1f, %.1f", ControlRotation.Pitch, ControlRotation.Yaw, ControlRotation.Roll);
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
	FVector StartLocation, StartVelocity;
	FVector TargetLocation, TargetVelocity;

	void Set(const FVector& InputStartLocation, const FVector& InputStartVelocity,
		const FVector& InputTargetLocation, const FVector& InputTargetVelocity)
	{
		StartLocation = InputStartLocation;
		StartVelocity = InputStartVelocity;
		TargetLocation = InputTargetLocation;
		TargetVelocity = InputTargetVelocity;
	}

	FVector InterpolateLocation(float InterpolationRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartVelocity, TargetLocation, TargetVelocity, InterpolationRatio);
	}

	FVector InterpolateVelocity(float InterpolationRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartVelocity, TargetLocation, TargetVelocity, InterpolationRatio);
	}
};


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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	void SetMaxVelocityFactor(const float NewMaxVelocityFactor);

	UFUNCTION(BlueprintCallable)
	FVector GetVelocity() const;

	UFUNCTION(BlueprintCallable)
	float GetCurrentYaw() const;


	// Begin : Replication 관련 코드 =============================================================================================
private:

	bool GetSharedWorldTime(float& SharedWorldTime) const;

	/**
	 * 클라이언트에서 서버로 입력을 전송하는 함수
	 * 처음에는 const FPlayerMovementObject& 형식으로 넘겨줬는데, 
	 * FPlayerMovementObject가 지역변수라서 서버에는 이상한 값이 전달됐다. 그냥 복사해서 넘기는 것으로 변경하였다. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMove(FPlayerMovementObject MovementObject);

	// bool CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const;

	void UpdateMovementState(const FPlayerMovementObject& MovementObject);

	/** 서버에서 리플리케이션되는 이동 정보 */
	UPROPERTY(ReplicatedUsing = OnRep_MovementState)
	FPlayerMovementState MovementState;

	UFUNCTION()
	void OnRep_MovementState();

	/** DeltaTime을 사용한 치트 방지 */
	float ClientSimulatedTimeInServer;


	/// SimulatedProxy 보간 관련

	/** 클라이언트에서 서버로부터 MovementState를 받았을 때, SimulatedProxy 을 시뮬레이션하는 함수 */
	void UpdateSimulatedProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */);

	/** 클라이언트에서의 상태가 서버와 많이 다를 때 강제로 서버 상태에 맞추는 함수 */
	void SetTargetMovementBySpline(FHermiteCubicSpline& Spline);

	/** 
	 * 클라이언트에서 SimulatedProxy 을 시뮬레이션할 때, 현재 움직임을 기반으로 미래의 움직임을 예측하는 함수
	 * 간단히 해당 움직임을 유지할 것이라고 예측한다. */
	void CalculateMovementByDeadReckoning(float ElapsedTime,
		FVector& CurrentLocation, FVector& CurrentVelocity) const;

	/** 클라이언트에서 Role이 ROLE_SimulatedProxy 일 때, 매 프레임 보간하는 함수 */
	void InterpolateFromClient(float DeltaTime);


	FHermiteCubicSpline CubicSpline;

	FQuat StartRotation;
	FQuat TargetRotation;

	float InterpolationCompletionTime;
	float CurrentInterpolationTime;
	//float InterpolationRatio;


	/// AutonomousProxy 보간 관련

	/** 클라이언트에서 서버로부터 MovementState를 받았을 때, AutonomousProxy 을 시뮬레이션하는 함수 */
	void UpdateAutonomousProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */);

	/** 서버에서 처리된 MovementObject를 삭제하는 함수 */
	void RemoveProcessedMovementObjects(float LastMovementObjectSharedWorldTime);

	/**
	 * 폰을 컨트롤하는 곳에서 정확한 시뮬레이션을 하기 위해 플레이어의 최신 이동 입력들을 저장하고 있다.
	 * 서버에서 처리되는대로 삭제된다. */
	TArray<FPlayerMovementObject> PendingMovementObjects;


	// End : Replication 관련 코드 =============================================================================================
	
	// Begin : Transform, Physics 관련 코드 ======================================================================================
private:

	/// 이동 관련

	void RefreshMovementVariable();

	float GetDeltaTranslationScalar(float DeltaTime, const FVector& CurrentVelocity) const;

	float GetResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetAirResistanceScalar(const float VelocityScalar /* Speed */) const;
	float GetFrictionResistanceScalar() const;

	void SimulateMovementObject(const FPlayerMovementObject& MovementObject);
	void SetVelocity(const FVector& NewVelocity);
	void UpdateVelocity(float DeltaTime, float ForwardMovementFactor, float RightMovementFactor);
	void ApplyResistanceToVelocity(float DeltaTime);
	void ApplyInputToVelocity(float DeltaTime, float ForwardMovementFactor, float RightMovementFactor);

	float ConvertToControlRotationRange(float TargetAngle) const;
	void UpdateTransform(float DeltaTime,
		float ForwardMovementFactor, float RightMovementFactor, const FRotator& ControlRotation);

	void UpdateRotation(float DeltaTranslationScalar, const FRotator& ControlRotation);
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
