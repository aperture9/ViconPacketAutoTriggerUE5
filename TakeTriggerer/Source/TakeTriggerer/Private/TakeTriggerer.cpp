// Copyright Epic Games, Inc. All Rights Reserved.

#include "TakeTriggerer.h"
#include "CoreGlobals.h"
#include "Common/UdpSocketBuilder.h"
#include "Sockets.h"
#include <Networking/Public/Interfaces/IPv4/IPv4Address.h>
#include <../Plugins/VirtualProduction/Takes/Source/TakeRecorder/Public/Recorder/TakeRecorderPanel.h>
#include <../Plugins/VirtualProduction/Takes/Source/TakeRecorder/Public/Recorder/TakeRecorderBlueprintLibrary.h>
#include "TakesCore\Public\TakeMetaData.h"
#include "EasyXMLParseManager.h"
#include "IPythonScriptPlugin.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#include "TakeTriggerSettings.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FTakeTriggererModule"

FTakeTriggererModule::FTakeTriggererModule()
{
}

void FTakeTriggererModule::StartupModule()
{
	RegisterUserSettings();
	
	//Postpone initialization to make sure modules are loaded. 
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTakeTriggererModule::OnEngineReady);
}

void FTakeTriggererModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	StopListening();
}

void FTakeTriggererModule::RegisterUserSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "Take Triggerer",
			LOCTEXT("ScriptsCollectionSettingsLabel", "Take Triggerer"),
			LOCTEXT("ScriptsCollectionSettingsDescr", "Configure the Take Triggerer Settings."),
			GetMutableDefault<UTakeTriggerSettings>());
	}
}

void FTakeTriggererModule::StartListening()
{

	// Store User Settings Obj
	UTakeTriggerSettings* TakeTriggerSettings = GetMutableDefault<UTakeTriggerSettings>();

	FString CmdLine;
	FConfigFile* EngineConfig = GConfig ? GConfig->FindConfigFileWithBaseName(FName(TEXT("Engine"))) : nullptr;
	if (!EngineConfig)
	{
		return;
	}

	TArray<FString> Settings;
	FString Setting;

	// Unicast endpoint setting
	EngineConfig->GetString(TEXT("/Script/UdpMessaging.UdpMessagingSettings"), TEXT("UnicastEndpoint"), Setting);

	// if the unicast endpoint port is bound, add 1 to the port for server 
	//Setting.ParseIntoArray(Settings, TEXT(":"), false);
	
	// Store UDP Messaging Plugin Obj
	//const UUdpMessagingSettings& Settings = *GetDefault<UUdpMessagingSettings>();
	
	FIPv4Endpoint UserSetEndpoint = FIPv4Endpoint::Any;
	bool bSuccess = FIPv4Endpoint::Parse(Setting, UserSetEndpoint);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Error, Failed to parse"));
	}

	// Actual Endpoint we will use, combo of the UDP messaging IP, with the user set port
	FIPv4Endpoint Endpoint(UserSetEndpoint.Address, TakeTriggerSettings->Port);
	
	//BUFFER SIZE
	const int32 BufferSize = 2 * 1024 * 1024;

	// Socket Name
	const FString SocketBuilderName = TEXT("ViconPacketListener");

	// Create Socket
	Socket = FUdpSocketBuilder(*SocketBuilderName)
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferSize);

	if (Socket)
	{
		// Socket Receiver
		FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
		SocketReceiver = new FUdpSocketReceiver(Socket, ThreadWaitTime, TEXT("Vicon Receiver"));
		SocketReceiver->OnDataReceived().BindRaw(this, &FTakeTriggererModule::OnViconPacketReceived);
		SocketReceiver->Start();
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("Error, Failed to start Vicon socket - likely your Endpoint is a invalid IP"));


}

void FTakeTriggererModule::StopListening()
{
	// Destroy Receiver
	if (SocketReceiver)
	{
		delete SocketReceiver;
		SocketReceiver = nullptr;
	}

	// Close Socket
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
}

void FTakeTriggererModule::OnViconPacketReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint)
{

	// Hard exit if we have disabled the system
	UTakeTriggerSettings* TakeTriggerSettings = GetMutableDefault<UTakeTriggerSettings>();
	if (!TakeTriggerSettings->bEnabled)
	{
		return;
	}

	// Convert Incoming Data to String with UTF To CHAR
	const FString Msg = UTF8_TO_TCHAR(InData->GetData());
	UE_LOG(LogTemp, Display, TEXT("Incoming Vicon Message: %s"), *Msg);

	static const FString ViconSlateNameKey(TEXT("Name"));
	static const FString ViconSlateNotesKey(TEXT("Notes"));
	static const FString ViconSlateDescKey(TEXT("Description"));

	FString SlateName, SlateDescription;

	// Tiny XML Version. Stops having to use the 3rd party plugin, but the plugin is just easier for now. Can switch to Unreals FastXML with more dev to work totally standalone.
	// FastXML is how the plugin works, but it requires more classes to setup, so future job!

	//tinyxml2::XMLDocument doc;
	//doc.Parse(TCHAR_TO_ANSI(*Msg));

	//tinyxml2::XMLElement* RootNode = doc.RootElement();
	//tinyxml2::XMLElement* MyNameNode = RootNode->FirstChildElement("Name");
	//FString SlateNameKey = UTF8_TO_TCHAR(MyNameNode->Value());
	//FString WorkingSlate = UTF8_TO_TCHAR(MyNameNode->FirstAttribute()->Value());
	//UE_LOG(LogTemp, Display, TEXT("TinyXML = %s"), *WorkingSlate);

	// Get Panel
	UTakeRecorderPanel* TakePanel = UTakeRecorderBlueprintLibrary::OpenTakeRecorderPanel();


	if (Msg.Contains("CaptureStart"))
	{
		EEasyXMLParserErrorCode ErrorCode;
		FString ErrorMsg;
		UEasyXMLParseManager* MyParser = NewObject<UEasyXMLParseManager>();
		UEasyXMLElement* MyXML = MyParser->LoadFromString(Msg, ErrorCode, ErrorMsg);
		if (MyXML)
		{
			SlateName = RequestAttributeFromViconXML(MyXML, ViconSlateNameKey);
			SlateDescription = RequestAttributeFromViconXML(MyXML, ViconSlateDescKey);
		}

		// Metadata stuff, for take name, desc, etc
		UTakeMetaData* Meta = TakePanel->GetTakeMetaData();
		Meta->SetSlate(SlateName);
		Meta->SetDescription(SlateDescription);
		Meta->SetTakeNumber(1);

		// Crashes if we do this in editor, so just use python to trigger it
		IPythonScriptPlugin::Get()->ExecPythonCommand(TEXT("import unreal; x = unreal.TakeRecorderBlueprintLibrary.get_take_recorder_panel(); x.start_recording()"));
		return;
	}

	if (Msg.Contains("CaptureStop"))
	{
		UE_LOG(LogTemp, Warning, TEXT("Stop Requested"));
		TakePanel->StopRecording();
		return;
	}
}

void FTakeTriggererModule::OnEngineReady()
{
	StartListening();
}


FString FTakeTriggererModule::RequestAttributeFromViconXML(UEasyXMLElement* Element, FString AttributeName)
{
	if (Element == nullptr)
	{
		return FString();
	}

	FString RequestedValue;

	const FString RootNodeName = TEXT("CaptureStart");
	//const FString RequestedAttributeName = TEXT("Name");
	const FString RequestedIndex = TEXT("0");
	const FString RequestedAttributeType = TEXT("VALUE");

	// Create Arg
	const FString Arg = RootNodeName + "." + AttributeName + "[" + RequestedIndex + "]" + ".@" + RequestedAttributeType;

	// Read - Cross  Fingers Time
	RequestedValue = Element->ReadString(Arg, FString());

	// Print
	UE_LOG(LogTemp, Warning, TEXT("Matts Magic Code: Name = %s"), *RequestedValue);

	return RequestedValue;

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTakeTriggererModule, TakeTriggerer)