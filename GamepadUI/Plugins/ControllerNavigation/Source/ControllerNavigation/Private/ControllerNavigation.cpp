// Gaslight Games Ltd, (C) 2016-2019. All rights reserved.

#include "ControllerNavigation.h"

// Settings
#include "Public/Config/ConNavConfig.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#endif

#define LOCTEXT_NAMESPACE "FControllerNavigationModule"

void FControllerNavigationModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	RegisterSettings();
}

void FControllerNavigationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if( UObjectInitialized() )
	{
		UnregisterSettings();
	}
}

bool FControllerNavigationModule::SupportsDynamicReloading()
{
	return true;
}

bool FControllerNavigationModule::HandleSettingsSaved()
{
	UControllerNavigationConfig* config = GetMutableDefault<UControllerNavigationConfig>();

	bool ResaveSettings = false;

	// You can put any validation code in here and resave the settings in case an invalid
	// value has been entered

	if( ResaveSettings )
	{
		config->SaveConfig();
	}

	return true;
}

void FControllerNavigationModule::RegisterSettings()
{
#if WITH_EDITOR
	/*
	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>( "Settings" ) )
	{
		// Create the new category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer( "Project" );

		SettingsContainer->DescribeCategory( "Controller Navigation",
											 NSLOCTEXT( "Controller Navigation", "RuntimeWDCategoryName", "Controller Navigation" ),
											 NSLOCTEXT( "Controller Navigation", "RuntimeWDCategoryDescription", "Global configuration options for Controller Navigation." ) );

		// Register the settings
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings( "Project", "Controller Navigation", "General",
																				NSLOCTEXT( "Controller Navigation", "RuntimeGeneralSettingsName", "General" ),
																				NSLOCTEXT( "Controller Navigation", "RuntimeGeneralSettingsDescription", "Global Settings for the Controller Navigation Module." ),
																				GetMutableDefault<UControllerNavigationConfig>() );

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if( SettingsSection.IsValid() )
		{
			SettingsSection->OnModified().BindRaw( this, &FControllerNavigationModule::HandleSettingsSaved );
		}
	}
	*/
#endif
}

void FControllerNavigationModule::UnregisterSettings()
{
	// Ensure to unregister all of your registered settings here, hot-reload would
	// otherwise yield unexpected results.
#if WITH_EDITOR
	/*
	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>( "Settings" ) )
	{
		SettingsModule->UnregisterSettings( "Project", "Controller Navigation", "General" );
	}
	*/
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE( FControllerNavigationModule, ControllerNavigation )

DEFINE_LOG_CATEGORY( ControllerNavigationLog );