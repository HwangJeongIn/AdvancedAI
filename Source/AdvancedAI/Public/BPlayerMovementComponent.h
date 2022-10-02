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

	/** �Է� �� ������ ���� ġƮ ���� */
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

	/** �������� �� ��Ŷ�� ���� �ð�, 1/2 RTT�� ����Ͽ� ���� �� ��Ȯ�� �ùķ��̼��� �ϱ� ���� ���ȴ�. */
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


	// Begin : Replication ���� �ڵ� =============================================================================================
private:

	bool GetSharedWorldTime(float& SharedWorldTime) const;

	/**
	 * Ŭ���̾�Ʈ���� ������ �Է��� �����ϴ� �Լ�
	 * ó������ const FPlayerMovementObject& �������� �Ѱ���µ�, 
	 * FPlayerMovementObject�� ���������� �������� �̻��� ���� ���޵ƴ�. �׳� �����ؼ� �ѱ�� ������ �����Ͽ���. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMove(FPlayerMovementObject MovementObject);

	// bool CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const;

	void UpdateMovementState(const FPlayerMovementObject& MovementObject);

	/** �������� ���ø����̼ǵǴ� �̵� ���� */
	UPROPERTY(ReplicatedUsing = OnRep_MovementState)
	FPlayerMovementState MovementState;

	UFUNCTION()
	void OnRep_MovementState();

	/** DeltaTime�� ����� ġƮ ���� */
	float ClientSimulatedTimeInServer;


	/// SimulatedProxy ���� ����

	/** Ŭ���̾�Ʈ���� �����κ��� MovementState�� �޾��� ��, SimulatedProxy �� �ùķ��̼��ϴ� �Լ� */
	void UpdateSimulatedProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */);

	/** Ŭ���̾�Ʈ������ ���°� ������ ���� �ٸ� �� ������ ���� ���¿� ���ߴ� �Լ� */
	void SetTargetMovementBySpline(FHermiteCubicSpline& Spline);

	/** 
	 * Ŭ���̾�Ʈ���� SimulatedProxy �� �ùķ��̼��� ��, ���� �������� ������� �̷��� �������� �����ϴ� �Լ�
	 * ������ �ش� �������� ������ ���̶�� �����Ѵ�. */
	void CalculateMovementByDeadReckoning(float ElapsedTime,
		FVector& CurrentLocation, FVector& CurrentVelocity) const;

	/** Ŭ���̾�Ʈ���� Role�� ROLE_SimulatedProxy �� ��, �� ������ �����ϴ� �Լ� */
	void InterpolateFromClient(float DeltaTime);


	FHermiteCubicSpline CubicSpline;

	FQuat StartRotation;
	FQuat TargetRotation;

	float InterpolationCompletionTime;
	float CurrentInterpolationTime;
	//float InterpolationRatio;


	/// AutonomousProxy ���� ����

	/** Ŭ���̾�Ʈ���� �����κ��� MovementState�� �޾��� ��, AutonomousProxy �� �ùķ��̼��ϴ� �Լ� */
	void UpdateAutonomousProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */);

	/** �������� ó���� MovementObject�� �����ϴ� �Լ� */
	void RemoveProcessedMovementObjects(float LastMovementObjectSharedWorldTime);

	/**
	 * ���� ��Ʈ���ϴ� ������ ��Ȯ�� �ùķ��̼��� �ϱ� ���� �÷��̾��� �ֽ� �̵� �Էµ��� �����ϰ� �ִ�.
	 * �������� ó���Ǵ´�� �����ȴ�. */
	TArray<FPlayerMovementObject> PendingMovementObjects;


	// End : Replication ���� �ڵ� =============================================================================================
	
	// Begin : Transform, Physics ���� �ڵ� ======================================================================================
private:

	/// �̵� ����

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
	 * N = �� �� (m/s^2)
	 * cN = �� �� (cm/s^2) */
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


	/// ȸ�� ����

	/** cm */
	UPROPERTY(EditDefaultsOnly)
	float MinTurningRadius;

	UPROPERTY(VisibleAnywhere)
	float CurrentYaw;

	// End : Transform, Physics ���� �ڵ� ======================================================================================
};
