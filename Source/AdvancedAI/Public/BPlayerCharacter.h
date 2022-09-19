// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;


UCLASS()
class ADVANCEDAI_API ABPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABPlayerCharacter();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void PostInitializeComponents() override;
	virtual FVector GetPawnViewLocation() const override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;


	float MovingFactor;
	float RotationFactor;

	float MinTurningRadius;
};
