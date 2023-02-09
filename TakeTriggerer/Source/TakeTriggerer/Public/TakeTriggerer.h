// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Common/UdpSocketReceiver.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

class UEasyXMLElement;

class FTakeTriggererModule : public IModuleInterface
{
public:

	FTakeTriggererModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterUserSettings();

	/** Socket used to listen for PSN packets. */
	FSocket* Socket;

	/** UDP receiver. */
	FUdpSocketReceiver* SocketReceiver;

	void StartListening();
	void StopListening();

	void OnViconPacketReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint);

	void OnEngineReady();

	FString RequestAttributeFromViconXML(UEasyXMLElement* Element, FString AttributeName);

	
};
