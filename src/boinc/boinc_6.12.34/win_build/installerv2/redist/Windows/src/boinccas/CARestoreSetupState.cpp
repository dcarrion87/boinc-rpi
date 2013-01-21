// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "stdafx.h"
#include "boinccas.h"
#include "CARestoreSetupState.h"

#define CUSTOMACTION_NAME               _T("CARestoreSetupState")
#define CUSTOMACTION_PROGRESSTITLE      _T("Restore the previous setups saved parameters.")


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
CARestoreSetupState::CARestoreSetupState(MSIHANDLE hMSIHandle) :
    BOINCCABase(hMSIHandle, CUSTOMACTION_NAME, CUSTOMACTION_PROGRESSTITLE)
{}


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
CARestoreSetupState::~CARestoreSetupState()
{
    BOINCCABase::~BOINCCABase();
}


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
UINT CARestoreSetupState::OnExecution()
{
    tstring     strInstallDirectory;
    tstring     strDataDirectory;
    tstring     strLaunchProgram;
    tstring     strEnableLaunchAtLogon;
    tstring     strEnableScreensaver;
    tstring     strEnableProtectedApplicationExecution;
    tstring     strEnableUseByAllUsers;
    tstring     strOverrideInstallDirectory;
    tstring     strOverrideDataDirectory;
    tstring     strOverrideLaunchProgram;
    tstring     strOverrideEnableLaunchAtLogon;
    tstring     strOverrideEnableScreensaver;
    tstring     strOverrideEnableProtectedApplicationExecution;
    tstring     strOverrideEnableUseByAllUsers;

    tstring     strSetupStateStored;

    GetRegistryValue( _T("SETUPSTATESTORED"), strSetupStateStored );
    if (strSetupStateStored == _T("TRUE")) {

        GetProperty( _T("OVERRIDE_INSTALLDIR"), strOverrideInstallDirectory );
        GetProperty( _T("OVERRIDE_DATADIR"), strOverrideDataDirectory );
        GetProperty( _T("OVERRIDE_LAUNCHPROGRAM"), strOverrideLaunchProgram );
        GetProperty( _T("OVERRIDE_ENABLELAUNCHATLOGON"), strOverrideEnableLaunchAtLogon );
        GetProperty( _T("OVERRIDE_ENABLESCREENSAVER"), strOverrideEnableScreensaver );
        GetProperty( _T("OVERRIDE_ENABLEPROTECTEDAPPLICATIONEXECUTION2"), strOverrideEnableProtectedApplicationExecution );
        GetProperty( _T("OVERRIDE_ENABLEUSEBYALLUSERS"), strOverrideEnableUseByAllUsers );

        GetRegistryValue( _T("INSTALLDIR"), strInstallDirectory );
        GetRegistryValue( _T("DATADIR"), strDataDirectory );
        GetRegistryValue( _T("LAUNCHPROGRAM"), strLaunchProgram );
        GetRegistryValue( _T("ENABLELAUNCHATLOGON"), strEnableLaunchAtLogon );
        GetRegistryValue( _T("ENABLESCREENSAVER"), strEnableScreensaver );
        GetRegistryValue( _T("ENABLEPROTECTEDAPPLICATIONEXECUTION2"), strEnableProtectedApplicationExecution );
        GetRegistryValue( _T("ENABLEUSEBYALLUSERS"), strEnableUseByAllUsers );

        if (strOverrideInstallDirectory.empty()) {
            SetProperty( _T("INSTALLDIR"), strInstallDirectory );
        } else {
            SetProperty( _T("INSTALLDIR"), strOverrideInstallDirectory );
        }

        if (strOverrideDataDirectory.empty()) {
            SetProperty( _T("DATADIR"), strDataDirectory );
        } else {
            SetProperty( _T("DATADIR"), strOverrideDataDirectory );
        }

        if (strOverrideLaunchProgram.empty()) {
            SetProperty( _T("LAUNCHPROGRAM"), strLaunchProgram );
        } else {
            SetProperty( _T("LAUNCHPROGRAM"), strOverrideLaunchProgram );
        }

        if (strOverrideEnableLaunchAtLogon.empty()) {
            SetProperty( _T("ENABLELAUNCHATLOGON"), strEnableLaunchAtLogon );
        } else {
            SetProperty( _T("ENABLELAUNCHATLOGON"), strOverrideEnableLaunchAtLogon );
        }

        if (strOverrideEnableScreensaver.empty()) {
            SetProperty( _T("ENABLESCREENSAVER"), strEnableScreensaver );
        } else {
            SetProperty( _T("ENABLESCREENSAVER"), strOverrideEnableScreensaver );
        }

        if (strOverrideEnableProtectedApplicationExecution.empty()) {
            SetProperty( _T("ENABLEPROTECTEDAPPLICATIONEXECUTION2"), strEnableProtectedApplicationExecution );
        } else {
            SetProperty( _T("ENABLEPROTECTEDAPPLICATIONEXECUTION2"), strOverrideEnableProtectedApplicationExecution );
        }

        if (strEnableUseByAllUsers.empty()) {
            SetProperty( _T("ENABLEUSEBYALLUSERS"), strEnableUseByAllUsers );
        } else {
            SetProperty( _T("ENABLEUSEBYALLUSERS"), strOverrideEnableUseByAllUsers );
        }
    }

    // If the Data Directory entry is empty then that means we need
    //   to populate it with the default value.
    GetProperty( _T("DATADIR"), strDataDirectory );
    if (strDataDirectory.empty()) {
        tstring strCommonApplicationDataFolder;

        // MSI already has this figured out, so lets get it.
        GetProperty( _T("CommonAppDataFolder"), strCommonApplicationDataFolder );

        // Construct the default value
        strDataDirectory = strCommonApplicationDataFolder + _T("BOINC\\");

        SetProperty( _T("DATADIR"), strDataDirectory );
    }

    return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
// 
// Function:    RestoreSetupState
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
UINT __stdcall RestoreSetupState(MSIHANDLE hInstall)
{
    UINT uiReturnValue = 0;

    CARestoreSetupState* pCA = new CARestoreSetupState(hInstall);
    uiReturnValue = pCA->Execute();
    delete pCA;

    return uiReturnValue;
}


const char *BOINC_RCSID_7bca879adb="$Id: CARestoreSetupState.cpp 18466 2009-06-19 17:49:38Z romw $";
