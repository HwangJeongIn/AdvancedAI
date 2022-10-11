// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BAction.generated.h"


class UWorld;
class UBActionComponent;


USTRUCT()
struct FActionBasicRunningData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool IsRunning;

	UPROPERTY()
	AActor* InstigatorActor;
};


/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBAction : public UObject
{
	GENERATED_BODY()
	
public:
	bool IsSupportedForNetworking() const override;

	UWorld* GetWorld() const override;

private:

	UFUNCTION(Category = "Action", BlueprintCallable)
	UBActionComponent* GetOwningActionComponent() const;

	UPROPERTY(Replicated)
	UBActionComponent* ActionComp;

public:
	void Initialize(UBActionComponent* NewActionComp);

	bool IsRunning() const;
	bool CanStart(AActor* InstigatorActor) const;
	bool CanStop(AActor* InstigatorActor) const;

	virtual void Start(AActor* InstigatorActor);
	virtual void Stop(AActor* InstigatorActor);

	bool Compare(const FName& InputName) const;

private:
	UFUNCTION()
	void OnRep_BasicRunningData();

	UPROPERTY(ReplicatedUsing = "OnRep_BasicRunningData")
	FActionBasicRunningData BasicRunningData;

	UPROPERTY(EditDefaultsOnly, Category = "Action")
	FName ActionName;
};
