// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BActionComponent.generated.h"


class UBAction;
enum class EActionType : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStateChanged, UBActionComponent*, OwningActionComp, UBAction*, Action);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDAI_API UBActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBActionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void InitializeActions();

	bool IsInitialized;


	UFUNCTION(Category = "Actions", BlueprintCallable)
	void AddAction(AActor* Instigator, TSubclassOf<UBAction> ActionClass);

	UFUNCTION(Category = "Actions", BlueprintCallable)
	void RemoveAction(UBAction* ActionToRemove);

	UFUNCTION(Category = "Actions", BlueprintCallable)
	UBAction* GetActionByClass(TSubclassOf<UBAction> ActionClass) const;

	UFUNCTION(Category = "Actions", BlueprintCallable)
	UBAction* GetAction(const EActionType& ActionType) const;

	UFUNCTION(Category = "Actions", BlueprintCallable)
	bool StartAction(AActor* Instigator, const EActionType& ActionType, bool WithoutActionStateValidation = false);

	UFUNCTION(Category = "Actions", BlueprintCallable)
	bool StartActionIfCan(AActor* Instigator, const EActionType& ActionType);

	UFUNCTION(Category = "Actions", BlueprintCallable)
	bool StopAction(AActor* Instigator, const EActionType& ActionType, bool WithoutActionStateValidation = false);

	UFUNCTION(Category = "Actions", BlueprintCallable)
	bool StopActionIfCan(AActor* Instigator, const EActionType& ActionType);


private:

	UFUNCTION(Server, Reliable)
	void ServerStartAction(AActor* Instigator, EActionType ActionName);

	UFUNCTION(Server, Reliable)
	void ServerStopAction(AActor* Instigator, EActionType ActionName);

	UPROPERTY(Category = "Actions", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UBAction>> DefaultActionClasses;

	UPROPERTY(Category = "Actions", VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	TArray<UBAction*> ActivatedActions;

public:

	UPROPERTY(BlueprintAssignable)
	FOnActionStateChanged OnActionStarted;

	UPROPERTY(BlueprintAssignable)
	FOnActionStateChanged OnActionStopped;
};
