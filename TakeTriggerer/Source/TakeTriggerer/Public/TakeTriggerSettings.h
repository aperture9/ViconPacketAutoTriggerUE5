// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TakeTriggerSettings.generated.h"

/**
 * 
 */
UCLASS(config = Engine, defaultconfig)
class TAKETRIGGERER_API UTakeTriggerSettings : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(config, EditAnywhere, Category = "Take Trigger", meta=(ConfigRestartRequired = true))
	int Port = 30;

	UPROPERTY(config, EditAnywhere, Category = "Take Trigger")
	bool bEnabled = true;
};
