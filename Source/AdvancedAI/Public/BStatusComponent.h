// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BStatusComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStatusChanged, AActor*, InstigatorActor, UBStatusComponent*, OwningStatusComp, float, NewValue, float, Delta);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDAI_API UBStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Status", BlueprintCallable)
	static UBStatusComponent* GetStatus(AActor* Actor);

	UFUNCTION(Category = "Status", BlueprintCallable)
	static bool IsAliveActor(AActor* Actor);

	UBStatusComponent();

public:

	UFUNCTION(Category = "Status", BlueprintCallable)
	bool IsAlive() const;

	UFUNCTION(Category = "Status", BlueprintCallable)
	bool IsFullHealth() const;

	UFUNCTION(Category = "Status", BlueprintCallable)
	float GetHealth() const;

	UFUNCTION(Category = "Status", BlueprintCallable)
	float GetMaxHealth() const;

	UFUNCTION(Category = "Status", BlueprintCallable)
	bool ChangeHealth(AActor* Instigator, float DeltaHealth);
	
	UFUNCTION(Category = "Status", BlueprintCallable)
	bool ForceKill(AActor* Instigator);

	UPROPERTY(Category = "Status", BlueprintAssignable)
	FOnStatusChanged OnHealthChanged;

private:

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHealthChanged(AActor* Instigator, float NewHealth, float DeltaHealth);

	UPROPERTY(Category = "Status", EditDefaultsOnly, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(Category = "Status", EditDefaultsOnly, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;
};
