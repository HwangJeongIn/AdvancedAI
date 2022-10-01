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

	/** �Է� �� ������ ���� ġƮ ���� */
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

	/** �������� �� ��Ŷ�� ���� �ð�, 1/2 RTT�� ����Ͽ� ���� �� ��Ȯ�� �ùķ��̼��� �ϱ� ���� ���ȴ�. */
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


	// Begin : Replication ���� �ڵ� =============================================================================================
private:

	bool GetSharedWorldTime(float& SharedWorldTime) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMove(const FPlayerMovementObject& MovementObject);

	// bool CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const;

	void UpdateMovementState(const FPlayerMovementObject& MovementObject);

	UFUNCTION()
	void OnRep_MovementState();

	/** �������� ���ø����̼ǵǴ� �̵� ���� */
	UPROPERTY(ReplicatedUsing = OnRep_MovementState)
	FPlayerMovementState MovementState;

	/** DeltaTime�� ����� ġƮ ���� */
	float ClientSimulatedTimeInServer;


	/// SimulatedProxy ���� ����

	/** Ŭ���̾�Ʈ���� �����κ��� MovementState�� �޾��� ��, SimulatedProxy �� �ùķ��̼��ϴ� �Լ� */
	void UpdateSimulatedProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */);

	/** 
	 * Ŭ���̾�Ʈ���� SimulatedProxy �� �ùķ��̼��� ��, ���� �������� ������� �̷��� �������� �����ϴ� �Լ�
	 * ������ �ش� �������� ������ ���̶�� �����Ѵ�. */
	void CalculateMovementByDeadReckoning(float DeltaTime);

	/** Ŭ���̾�Ʈ���� Role�� ROLE_SimulatedProxy �� ��, �� ������ �����ϴ� �Լ� */
	void InterpolateFromClient(float DeltaTime);

	/** 
	 * Ŭ���̾�Ʈ���� SimulatedProxy �� �ùķ��̼��� ��, �����ϴ� �뵵�� ���Ǵ� ����
	 * �ð��� ���� Location�� ����ؼ� ���Ѵ�.
	 
	 */

	FVector TargetVelocity;
	FVector TargetLocation;
	FQuat TargetRotation;


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
