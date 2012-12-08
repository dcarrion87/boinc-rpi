// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SkinManager.h"
#endif

#include "stdwx.h"
#include "diagnostics.h"
#include "parse.h"
#include "util.h"
#include "error_numbers.h"
#include "miofile.h"
#include "BOINCGUIApp.h"
#include "BOINCBaseFrame.h"
#include "SkinManager.h"
#include "version.h"


////@begin XPM images
#include "res/skins/default/graphic/background_image.xpm"
#include "res/skins/default/graphic/dialog_background_image.xpm"
#include "res/skins/default/graphic/project_image.xpm"
#include "res/skins/default/graphic/workunit_animation_image.xpm"
#include "res/skins/default/graphic/workunit_running_image.xpm"
#include "res/skins/default/graphic/workunit_suspended_image.xpm"
#include "res/skins/default/graphic/workunit_waiting_image.xpm"
#include "res/boinc.xpm"
#include "res/boinc32.xpm"
#include "res/boincdisconnect.xpm"
#include "res/boincsnooze.xpm"
#include "res/boinc_logo.xpm"
////@end XPM images


// Flag to enable the various error messages
static bool show_error_msgs = false;


IMPLEMENT_DYNAMIC_CLASS(CSkinItem, wxObject)


CSkinItem::CSkinItem() {
}


CSkinItem::~CSkinItem() {
}


wxColour CSkinItem::ParseColor(wxString strColor) {
    long red, green, blue;
    wxStringTokenizer tkz(strColor, wxT(":"), wxTOKEN_RET_EMPTY);
    wxString(tkz.GetNextToken()).ToLong(&red);
	wxString(tkz.GetNextToken()).ToLong(&green);
	wxString(tkz.GetNextToken()).ToLong(&blue);
    return wxColour((unsigned char)red, (unsigned char)green, (unsigned char)blue);
}


IMPLEMENT_DYNAMIC_CLASS(CSkinImage, CSkinItem)


CSkinImage::CSkinImage() {
    Clear();
}


CSkinImage::~CSkinImage() {
    Clear();
}


void CSkinImage::Clear() {
    m_strDesiredBitmap.Clear();
    m_strDesiredBackgroundColor.Clear();
    m_bmpBitmap = wxNullBitmap;
    m_colBackgroundColor = wxNullColour;
}


int CSkinImage::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</image>")) break;
        else if (parse_str(buf, "<imagesrc>", strBuffer)) {
            if (strBuffer.length()) {
                m_strDesiredBitmap = wxString(
                    wxGetApp().GetSkinManager()->ConstructSkinPath() +
                    wxString(strBuffer.c_str(), wxConvUTF8)
                );
            }
            continue;
        } else if (parse_str(buf, "<background_color>", strBuffer)) {
            if (strBuffer.length()) {
                m_strDesiredBackgroundColor = wxString(strBuffer.c_str(), wxConvUTF8);
            }
            continue;
        }
    }

    return BOINC_SUCCESS;
}


wxBitmap* CSkinImage::GetBitmap() {
    Validate();
    return &m_bmpBitmap; 
}


wxColour* CSkinImage::GetBackgroundColor() {
    Validate();
    return &m_colBackgroundColor;
}


bool CSkinImage::SetDefaults(wxString strComponentName, const char** ppDefaultBitmap) {
    m_strComponentName = strComponentName;
    m_ppDefaultBitmap = ppDefaultBitmap;
    return true;
}


bool CSkinImage::SetDefaults(wxString strComponentName, const char** ppDefaultBitmap, wxString strBackgroundColor) {
    m_strComponentName = strComponentName;
    m_ppDefaultBitmap = ppDefaultBitmap;
    m_strDefaultBackgroundColor = strBackgroundColor;
    return true;
}


bool CSkinImage::Validate() {
    if (!m_bmpBitmap.Ok()) {
        if (!m_strDesiredBitmap.IsEmpty()) {
            m_bmpBitmap = wxBitmap(wxImage(m_strDesiredBitmap, wxBITMAP_TYPE_ANY));
        }
        if (!m_bmpBitmap.Ok()) {
            if (show_error_msgs) {
                fprintf(stderr, "Skin Manager: Failed to load '%s' image. Using default.\n", (const char *)m_strComponentName.mb_str());
            }
            m_bmpBitmap = wxBitmap(m_ppDefaultBitmap);
            wxASSERT(m_bmpBitmap.Ok());
        }
    }
    if (!m_colBackgroundColor.Ok()) {
        if (!m_strDesiredBackgroundColor.IsEmpty()) {
            m_colBackgroundColor = ParseColor(m_strDesiredBackgroundColor);
        }
        if (!m_colBackgroundColor.Ok()) {
            if (show_error_msgs) {
                fprintf(stderr, "Skin Manager: Failed to load '%s' background color. Using default.\n", (const char *)m_strComponentName.mb_str());
            }
            m_colBackgroundColor = ParseColor(m_strDefaultBackgroundColor);
            wxASSERT(m_colBackgroundColor.Ok());
        }
    }
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinIcon, CSkinItem)


CSkinIcon::CSkinIcon() {
    Clear();
}


CSkinIcon::~CSkinIcon() {
    Clear();
}


void CSkinIcon::Clear() {
    m_strDesiredIcon.Clear();
    m_strDesiredTransparencyMask.Clear();
    m_icoIcon = wxNullIcon;
}


int CSkinIcon::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</image>")) break;
        else if (parse_str(buf, "<imagesrc>", strBuffer)) {
            if (strBuffer.length()) {
                m_strDesiredIcon = wxString(
                    wxGetApp().GetSkinManager()->ConstructSkinPath() +
                    wxString(strBuffer.c_str(), wxConvUTF8)
                );
            }
            continue;
        } else if (parse_str(buf, "<transparency_mask>", strBuffer)) {
            if (strBuffer.length()) {
                m_strDesiredTransparencyMask = wxString(strBuffer.c_str(), wxConvUTF8);
            }
            continue;
        }
    }

    return BOINC_SUCCESS;
}


wxIcon* CSkinIcon::GetIcon() {
    Validate();
    return &m_icoIcon;
}


bool CSkinIcon::SetDefaults(wxString strComponentName, const char** ppDefaultIcon) {
    m_strComponentName = strComponentName;
    m_ppDefaultIcon = ppDefaultIcon;
    return true;
}


bool CSkinIcon::Validate() {
    if (!m_icoIcon.Ok()) {
        if (!m_strDesiredIcon.IsEmpty()) {
            // Configure bitmap object with optional transparency mask
            wxImage img = wxImage(m_strDesiredIcon, wxBITMAP_TYPE_ANY);
            wxBitmap bmp = wxBitmap(img);
            // If PNG file has alpha channel use it as mask & ignore <transparency_mask> tag 
            if (!(m_strDesiredTransparencyMask.IsEmpty() || img.HasAlpha())) {
                bmp.SetMask(new wxMask(bmp, ParseColor(m_strDesiredTransparencyMask)));
            }
            // Now set the icon object using the newly created bitmap with optional transparency mask
            m_icoIcon.CopyFromBitmap(bmp);
        }
        if (!m_icoIcon.Ok()) {
            if (show_error_msgs) {
                fprintf(stderr, "Skin Manager: Failed to load '%s' icon. Using default.\n", (const char *)m_strComponentName.mb_str());
            }
            m_icoIcon = wxIcon(m_ppDefaultIcon);
            wxASSERT(m_icoIcon.Ok());
        }
    }
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinSimple, CSkinItem)


CSkinSimple::CSkinSimple() {
    Clear();
}


CSkinSimple::~CSkinSimple() {
    Clear();
}


void CSkinSimple::Clear() {
	m_BackgroundImage.Clear();
    m_DialogBackgroundImage.Clear();
    m_ProjectImage.Clear();
	m_StaticLineColor = wxNullColour;
    m_WorkunitAnimationImage.Clear();
    m_WorkunitRunningImage.Clear();
    m_WorkunitSuspendedImage.Clear();
    m_WorkunitWaitingImage.Clear();
    m_iPanelOpacity = DEFAULT_OPACITY;
}


int CSkinSimple::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</simple>")) break;
        else if (match_tag(buf, "<background_image>")) {
            m_BackgroundImage.Parse(in);
            continue;
        } else if (match_tag(buf, "<dialog_background_image>")) {
            m_DialogBackgroundImage.Parse(in);
            continue;
        } else if (match_tag(buf, "<project_image>")) {
            m_ProjectImage.Parse(in);
            continue;
        } else if (parse_str(buf, "<static_line_color>", strBuffer)) {
            m_StaticLineColor = ParseColor(wxString(strBuffer.c_str(), wxConvUTF8));
            continue;
        } else if (match_tag(buf, "<workunit_animation_image>")) {
            m_WorkunitAnimationImage.Parse(in);
            continue;
        } else if (match_tag(buf, "<workunit_running_image>")) {
            m_WorkunitRunningImage.Parse(in);
            continue;
        } else if (match_tag(buf, "<workunit_suspended_image>")) {
            m_WorkunitSuspendedImage.Parse(in);
            continue;
        } else if (match_tag(buf, "<workunit_waiting_image>")) {
            m_WorkunitWaitingImage.Parse(in);
            continue;
        } else if (parse_int(buf, "<panel_opacity>", m_iPanelOpacity)) {
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


bool CSkinSimple::InitializeDelayedValidation() {
    m_BackgroundImage.SetDefaults(
        wxT("background"), (const char**)background_image_xpm, wxT("211:211:211")
    );
    m_DialogBackgroundImage.SetDefaults(
        wxT("dialog background"), (const char**)dialog_background_image_xpm, wxT("211:211:211")
    );
    m_ProjectImage.SetDefaults(
        wxT("project"), (const char**)project_image_xpm
    );
     m_WorkunitAnimationImage.SetDefaults(
        wxT("workunit animation"), (const char**)workunit_animation_image_xpm
    );
     m_WorkunitRunningImage.SetDefaults(
        wxT("workunit running"), (const char**)workunit_running_image_xpm
    );
     m_WorkunitSuspendedImage.SetDefaults(
        wxT("workunit suspended"), (const char**)workunit_suspended_image_xpm
    );
     m_WorkunitWaitingImage.SetDefaults(
        wxT("workunit waiting"), (const char**)workunit_waiting_image_xpm
    );
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinAdvanced, CSkinItem)


CSkinAdvanced::CSkinAdvanced() {
    Clear();
}


CSkinAdvanced::~CSkinAdvanced() {
    Clear();
}


void CSkinAdvanced::Clear() {
    m_bIsBranded = false;
    m_strApplicationName = wxEmptyString;
    m_strApplicationShortName = wxEmptyString;
    m_iconApplicationIcon.Clear();
    m_iconApplicationIcon32.Clear();
    m_iconApplicationDisconnectedIcon.Clear();
    m_iconApplicationSnoozeIcon.Clear();
    m_bitmapApplicationLogo = wxNullBitmap;
    m_strOrganizationName = wxEmptyString;
    m_strOrganizationWebsite = wxEmptyString;
    m_strOrganizationHelpUrl = wxEmptyString;
    m_bDefaultTabSpecified = false;
    m_iDefaultTab = 0;
    m_strExitMessage = wxEmptyString;
}


int CSkinAdvanced::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</advanced>")) break;
        else if (parse_bool(buf, "is_branded", m_bIsBranded)) continue;
        else if (parse_str(buf, "<application_name>", strBuffer)) {
            m_strApplicationName = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        } else if (parse_str(buf, "<application_short_name>", strBuffer)) {
            m_strApplicationShortName = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        } else if (match_tag(buf, "<application_icon>")) {
            m_iconApplicationIcon.Parse(in);
            continue;
        } else if (match_tag(buf, "<application_icon32>")) {
            m_iconApplicationIcon32.Parse(in);
            continue;
        } else if (match_tag(buf, "<application_disconnected_icon>")) {
            m_iconApplicationDisconnectedIcon.Parse(in);
            continue;
        } else if (match_tag(buf, "<application_snooze_icon>")) {
            m_iconApplicationSnoozeIcon.Parse(in);
            continue;
        } else if (parse_str(buf, "<application_logo>", strBuffer)) {
            if(strBuffer.length()) {
                wxString str = wxString(
                    wxGetApp().GetSkinManager()->ConstructSkinPath() +
                    wxString(strBuffer.c_str(), wxConvUTF8)
                );
                m_bitmapApplicationLogo = wxBitmap(wxImage(str.c_str(), wxBITMAP_TYPE_ANY));
            }
            continue;
        } else if (parse_str(buf, "<organization_name>", strBuffer)) {
            m_strOrganizationName = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        } else if (parse_str(buf, "<organization_website>", strBuffer)) {
            m_strOrganizationWebsite = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        } else if (parse_str(buf, "<organization_help_url>", strBuffer)) {
            m_strOrganizationHelpUrl = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        } else if (parse_int(buf, "<open_tab>", m_iDefaultTab)) {
            m_bDefaultTabSpecified = true;
            continue;
        } else if (parse_str(buf, "<exit_message>", strBuffer)) {
            m_strExitMessage = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


wxString CSkinAdvanced::GetApplicationName() {
    wxString strApplicationName = m_strApplicationName;
#ifdef BOINC_PRERELEASE
        strApplicationName += wxT(" (Pre-release)");
#endif
    return strApplicationName;
}


wxString CSkinAdvanced::GetApplicationShortName() {
    return m_strApplicationShortName;
}


wxIcon* CSkinAdvanced::GetApplicationIcon() {
    return m_iconApplicationIcon.GetIcon();
}

wxIcon* CSkinAdvanced::GetApplicationIcon32() {
    return m_iconApplicationIcon32.GetIcon();
}

wxIcon* CSkinAdvanced::GetApplicationDisconnectedIcon() { 
    return m_iconApplicationDisconnectedIcon.GetIcon();
}


wxIcon* CSkinAdvanced::GetApplicationSnoozeIcon() {
    return m_iconApplicationSnoozeIcon.GetIcon();
}


wxBitmap* CSkinAdvanced::GetApplicationLogo() {
    return &m_bitmapApplicationLogo;
}


wxString CSkinAdvanced::GetOrganizationName() { 
    return m_strOrganizationName;
}


wxString CSkinAdvanced::GetOrganizationWebsite() {
    return m_strOrganizationWebsite;
}


wxString CSkinAdvanced::GetOrganizationHelpUrl() {
    return m_strOrganizationHelpUrl;
}


int CSkinAdvanced::GetDefaultTab() { 
    return m_iDefaultTab;
}


wxString CSkinAdvanced::GetExitMessage() { 
    return m_strExitMessage;
}


bool CSkinAdvanced::IsBranded() { 
    return m_bIsBranded;
}


bool CSkinAdvanced::InitializeDelayedValidation() {
    if (m_strApplicationName.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Application name was not defined. Using default.\n");
        }
        m_strApplicationName = wxT("BOINC Manager");
        wxASSERT(!m_strApplicationName.IsEmpty());
    }
    if (m_strApplicationShortName.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Application short name was not defined. Using default.\n");
        }
        m_strApplicationShortName = wxT("BOINC");
        wxASSERT(!m_strApplicationShortName.IsEmpty());
    }
    m_iconApplicationIcon.SetDefaults(wxT("application"), (const char**)boinc_xpm);
    m_iconApplicationIcon32.SetDefaults(wxT("application"), (const char**)boinc32_xpm);
    m_iconApplicationDisconnectedIcon.SetDefaults(wxT("application disconnected"), (const char**)boincdisconnect_xpm);
    m_iconApplicationSnoozeIcon.SetDefaults(wxT("application snooze"), (const char**)boincsnooze_xpm);
    if (!m_bitmapApplicationLogo.Ok()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Failed to load application logo. Using default.\n");
        }
        m_bitmapApplicationLogo = wxBitmap((const char**)boinc_logo_xpm);
        wxASSERT(m_bitmapApplicationLogo.Ok());
    }
    if (m_strOrganizationName.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Organization name was not defined. Using default.\n");
        }
        m_strOrganizationName = wxT("Space Sciences Laboratory, U.C. Berkeley");
        wxASSERT(!m_strOrganizationName.IsEmpty());
    }
    if (m_strOrganizationWebsite.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Organization web site was not defined. Using default.\n");
        }
        m_strOrganizationWebsite = wxT("http://boinc.berkeley.edu");
        wxASSERT(!m_strOrganizationWebsite.IsEmpty());
    }
    if (m_strOrganizationHelpUrl.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Organization help url was not defined. Using default.\n");
        }
        m_strOrganizationHelpUrl = wxT("http://boinc.berkeley.edu/manager_links.php");
        wxASSERT(!m_strOrganizationHelpUrl.IsEmpty());
    }
    if (!m_bDefaultTabSpecified) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Default tab was not defined. Using default.\n");
        }
        m_bDefaultTabSpecified = true;
        m_iDefaultTab = 0;
    }
    if (m_strExitMessage.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Exit message was not defined. Using default.\n");
        }
        m_strExitMessage = wxEmptyString;
    }
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinWizardATP, CSkinItem)


CSkinWizardATP::CSkinWizardATP() {
    Clear();
}


CSkinWizardATP::~CSkinWizardATP() {
    Clear();
}


void CSkinWizardATP::Clear() {
    m_strTitle = wxEmptyString;
}


int CSkinWizardATP::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</attach_to_project>")) break;
        else if (parse_str(buf, "<title>", strBuffer)) {
            m_strTitle = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


bool CSkinWizardATP::InitializeDelayedValidation() {
    if (m_strTitle.IsEmpty()) {
        if (show_error_msgs) {
            fprintf(stderr, "Skin Manager: Add project wizard title was not defined. Using default.\n");
        }
        m_strTitle = wxT("BOINC Manager");
        wxASSERT(!m_strTitle.IsEmpty());
    }
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinWizardATAM, CSkinItem)


CSkinWizardATAM::CSkinWizardATAM() {
    Clear();
}


CSkinWizardATAM::~CSkinWizardATAM() {
    Clear();
}


void CSkinWizardATAM::Clear() {
    m_strAccountInfoMessage = wxEmptyString;
}


int CSkinWizardATAM::Parse(MIOFILE& in) {
    char buf[256];
    std::string strBuffer;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</attach_to_account_manager>")) break;
        else if (parse_str(buf, "<account_info_message>", strBuffer)) {
            m_strAccountInfoMessage = wxString(strBuffer.c_str(), wxConvUTF8);
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


bool CSkinWizardATAM::InitializeDelayedValidation() {
    return true;
}


IMPLEMENT_DYNAMIC_CLASS(CSkinWizards, CSkinItem)


CSkinWizards::CSkinWizards() {
    Clear();
}


CSkinWizards::~CSkinWizards() {
    Clear();
}


void CSkinWizards::Clear() {
    m_AttachToProjectWizard.Clear();
    m_AttachToAccountManagerWizard.Clear();
}


int CSkinWizards::Parse(MIOFILE& in) {
    char buf[256];

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</wizards>")) break;
        else if (match_tag(buf, "<attach_to_project>")) {
            m_AttachToProjectWizard.Parse(in);
            continue;
        } else if (match_tag(buf, "<attach_to_account_manager>")) {
            m_AttachToAccountManagerWizard.Parse(in);
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


bool CSkinWizards::InitializeDelayedValidation() {
    return m_AttachToProjectWizard.InitializeDelayedValidation() && 
           m_AttachToAccountManagerWizard.InitializeDelayedValidation();
}


IMPLEMENT_DYNAMIC_CLASS(CSkinManager, CSkinItem)


CSkinManager::CSkinManager() {}


CSkinManager::CSkinManager(bool debugSkins) {
    show_error_msgs = debugSkins;
    Clear();
}


CSkinManager::~CSkinManager() {
    Clear();
}

bool CSkinManager::ReloadSkin(wxString strSkin) {
    int      retval = ERR_XML_PARSE;
    FILE*    p;
    MIOFILE  mf;

    // This fixes a (rare) crash bug
    if (strSkin.IsEmpty()) {
        strSkin = GetDefaultSkinName();
    }
    
    // Clear out all the old stuff 
    Clear();

    // Set the default skin back to Default
    m_strSelectedSkin = strSkin;

    // TODO: Eliminate the <en> tags: localization is no longer in skin files.
    p = fopen((const char*)ConstructSkinFileName().mb_str(wxConvUTF8), "r");
    if (p) {
        mf.init_file(p);
        retval = Parse(mf, wxT("en"));
        fclose(p);
    }

    if (retval && show_error_msgs) {
        fprintf(stderr, "Skin Manager: Failed to load skin '%s'.\n", (const char *)ConstructSkinFileName().mb_str(wxConvUTF8));
    }

    InitializeDelayedValidation();

    // Tell whichever UI elements that are loaded to reload the
    //   skinable resources they use.
    wxGetApp().FireReloadSkin();

    return true;
}

wxArrayString& CSkinManager::GetCurrentSkins() {
    unsigned int i;
    wxString     strSkinLocation = wxString(GetSkinsLocation() + wxFileName::GetPathSeparator());
    wxString     strSkinFileName = wxString(wxFileName::GetPathSeparator() + GetSkinFileName());
    wxString     strBuffer;

    // Initialize array
    m_astrSkins.Clear();

    // Go get all the valid skin directories.
    wxDir::GetAllFiles(strSkinLocation, &m_astrSkins, wxString(wxT("*") + GetSkinFileName()));

    // Trim out the path information for all the entries
    for (i = 0; i < m_astrSkins.GetCount(); i++) {
        strBuffer = m_astrSkins[i];

        strBuffer = strBuffer.Remove(0, strSkinLocation.Length());
        strBuffer = strBuffer.Remove(strBuffer.Find(strSkinFileName.c_str()), strSkinFileName.Length());

        // Special case: 'Default' to mean the embedded default skin.
        //   remove any duplicate entries
        if (GetDefaultSkinName() != strBuffer) {
            m_astrSkins[i] = strBuffer;
        } else {
            m_astrSkins.RemoveAt(i);
            i--;
        }
    }

    // Insert the 'Default' entry into the skins list.
    m_astrSkins.Insert(GetDefaultSkinName(), 0);

    // return the current list of skins
    return m_astrSkins;
}


wxString CSkinManager::GetDefaultSkinName() {
    return wxString(wxT("Default"));
}


wxString CSkinManager::ConstructSkinFileName() {
    return wxString(
        GetSkinsLocation() + 
        wxString(wxFileName::GetPathSeparator()) +
        m_strSelectedSkin + 
        wxString(wxFileName::GetPathSeparator()) +
        GetSkinFileName()
    );
}


wxString CSkinManager::ConstructSkinPath() {
    return wxString(
        GetSkinsLocation() + 
        wxString(wxFileName::GetPathSeparator()) +
        m_strSelectedSkin + 
        wxString(wxFileName::GetPathSeparator())
    );
}


wxString CSkinManager::GetSkinFileName() {
    // Construct path to skins directory
    return wxString(wxT("skin.xml"));
}


wxString CSkinManager::GetSkinsLocation() {
    // Construct path to skins directory
    wxString strSkinLocation = wxEmptyString;

#ifdef __WXMSW__
    strSkinLocation  = wxGetApp().GetRootDirectory();
    strSkinLocation += wxFileName::GetPathSeparator();
    strSkinLocation += wxT("skins");
#else
    strSkinLocation = wxString(wxGetCwd() + wxString(wxFileName::GetPathSeparator()) + wxT("skins"));
#endif

    return strSkinLocation;
}


void CSkinManager::Clear() {
    m_SimpleSkin.Clear();
    m_AdvancedSkin.Clear();
    m_WizardsSkin.Clear();

    m_astrSkins.Clear();
    m_strSelectedSkin.Clear();
}


int CSkinManager::Parse(MIOFILE& in, wxString strDesiredLocale) {
    char     buf[256];
    wxString strLocaleStartTag;
    wxString strLocaleEndTag;
    bool     bLocaleFound = false;

    // Construct the start and end tags for the locale we want.
    strLocaleStartTag.Printf(wxT("<%s>"), strDesiredLocale.c_str());
    strLocaleEndTag.Printf(wxT("</%s>"), strDesiredLocale.c_str());

    // TODO: Eliminate the <en> tags: localization is no longer in skin files.
    // Look for the begining of the desired locale.
    while (in.fgets(buf, 256)) {
        if (match_tag(buf, (const char*)strLocaleStartTag.mb_str(wxConvUTF8))) {
            bLocaleFound = true;
            break;
        }
    }

    if (!bLocaleFound) return ERR_XML_PARSE;

    while (in.fgets(buf, 256)) {
        if (match_tag(buf, (const char*)strLocaleStartTag.mb_str(wxConvUTF8))) break;
        else if (match_tag(buf, "<simple>")) {
            m_SimpleSkin.Parse(in);
            continue;
        } else if (match_tag(buf, "<advanced>")) {
            m_AdvancedSkin.Parse(in);
            continue;
        } else if (match_tag(buf, "<wizards>")) {
            m_WizardsSkin.Parse(in);
            continue;
        }
    }

    InitializeDelayedValidation();

    return 0;
}


bool CSkinManager::InitializeDelayedValidation() {
    return m_SimpleSkin.InitializeDelayedValidation() && 
           m_AdvancedSkin.InitializeDelayedValidation() && 
           m_WizardsSkin.InitializeDelayedValidation();
}

