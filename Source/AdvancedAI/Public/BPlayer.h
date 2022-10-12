// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BPlayer.generated.h"


class UCapsuleComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UBPlayerMovementComponent;
class UBStatusComponent;
class UBActionComponent;
class UBPlayerAnimInstance;


UCLASS()
class ADVANCEDAI_API ABPlayer : public APawn
{
	GENERATED_BODY()

public:
	ABPlayer();

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;


	/** Movement */
public:
	virtual FVector GetVelocity() const override;

	UFUNCTION(BlueprintCallable)
	float GetCurrentYaw() const;
	float GetForwardMovementFactor() const;
	float GetRightMovementFactor() const;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBPlayerMovementComponent* PlayerMovementComp;

	float ForwardMovementFactor;
	float RightMovementFactor;


	/** Status */
private:
	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UBStatusComponent* OwningStatusComp, float NewHealth, float DeltaHealth);

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBStatusComponent* StatusComp;


	/** Action */
public:
	UBPlayerAnimInstance* GetPlayerAnimInstance();

	void InitializeActionComponent();

private:
	void PrimaryAttackStart();
	void PrimaryAttackStop();

	void SecondaryAttack();
	void Dash();

	void SprintStart();
	void SprintEnd();

	UFUNCTION()
	void OnPrimaryAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBActionComponent* ActionComp;

	UPROPERTY(Category = "Action", VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"))
	UBPlayerAnimInstance* PlayerAnimInstance;

	UPROPERTY(Category = "Action", VisibleAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"))
	bool IsAttacking;
};
