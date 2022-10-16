// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BAICharacter.generated.h"


class UBStatusComponent;
class UBActionComponent;
class UBSightComponent;


UCLASS()
class ADVANCEDAI_API ABAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABAICharacter();

protected:
	virtual void BeginPlay() override;


	/** Status */
private:
	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UBStatusComponent* OwningStatusComp, float NewHealth, float DeltaHealth);

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBStatusComponent* StatusComp;

	/** Action */
private:

	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBActionComponent* ActionComp;


	/** Sight */
private:
	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBSightComponent* SightComp;
};
