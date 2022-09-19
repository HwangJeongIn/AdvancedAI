// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BPlayer.generated.h"


class UCapsuleComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;


UCLASS()
class ADVANCEDAI_API ABPlayer : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	void MoveForward(float Value);
	void MoveRight(float Value);

private:
	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;



	// 시뮬레이션 테스트
	/*
	float GetAirResistance();
	float GetFrictionResistance();

	UPROPERTY(EditDefaultsOnly)
	float DefaultMass;


	UPROPERTY(EditDefaultsOnly)
	float MovingFactor;

	UPROPERTY(EditDefaultsOnly)
	float DefaultMovingForce;

	UPROPERTY(EditDefaultsOnly)
	float RotationFactor;

	UPROPERTY(EditDefaultsOnly)
	float DefaultTurningRadius;

	UPROPERTY(EditDefaultsOnly)
	float DragCoefficient;

	UPROPERTY(EditDefaultsOnly)
	float FrictionCoefficient;

	UPROPERTY(VisibleAnywhere)
	float DefaultGravity;
	*/
};
