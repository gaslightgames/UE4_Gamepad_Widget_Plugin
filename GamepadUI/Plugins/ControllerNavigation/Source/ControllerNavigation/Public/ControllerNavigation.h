// Gaslight Games Ltd, (C) 2016-2019. All rights reserved.

#pragma once

#include "Engine.h"

class FControllerNavigationModule : public FDefaultGameModuleImpl
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override;

protected:

	bool HandleSettingsSaved();
	void RegisterSettings();
	void UnregisterSettings();
};

DECLARE_LOG_CATEGORY_EXTERN( ControllerNavigationLog, Log, All );