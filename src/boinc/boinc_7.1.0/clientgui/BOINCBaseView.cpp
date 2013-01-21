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
#pragma implementation "BOINCBaseView.h"
#endif

#include "stdwx.h"
#include "BOINCGUIApp.h"
#include "MainDocument.h"
#include "BOINCBaseView.h"
#include "BOINCTaskCtrl.h"
#include "BOINCListCtrl.h"
#include "Events.h"

#include "res/boinc.xpm"
#include "res/sortascending.xpm"
#include "res/sortdescending.xpm"


IMPLEMENT_DYNAMIC_CLASS(CBOINCBaseView, wxPanel)


CBOINCBaseView::CBOINCBaseView() {}

CBOINCBaseView::CBOINCBaseView(wxNotebook* pNotebook) :
    wxPanel(pNotebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
{
    m_bProcessingTaskRenderEvent = false;
    m_bProcessingListRenderEvent = false;

    m_bForceUpdateSelection = true;
    m_bIgnoreUIEvents = false;
    m_bNeedSort = false;

    //
    // Setup View
    //
    m_pTaskPane = NULL;
    m_pListPane = NULL;
    m_iProgressColumn = -1;
    m_iSortColumn = -1;
    m_SortArrows = NULL;
    
    SetName(GetViewName());
    SetAutoLayout(TRUE);

#if BASEVIEW_STRIPES    
    m_pWhiteBackgroundAttr = NULL;
    m_pGrayBackgroundAttr = NULL;
#endif
}


CBOINCBaseView::CBOINCBaseView(wxNotebook* pNotebook, wxWindowID iTaskWindowID, int iTaskWindowFlags, wxWindowID iListWindowID, int iListWindowFlags) :
    wxPanel(pNotebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
{
    m_bProcessingTaskRenderEvent = false;
    m_bProcessingListRenderEvent = false;

    m_bForceUpdateSelection = true;
    m_bIgnoreUIEvents = false;

    //
    // Setup View
    //
    m_pTaskPane = NULL;
    m_pListPane = NULL;

    SetName(GetViewName());
    SetAutoLayout(TRUE);

    wxFlexGridSizer* itemFlexGridSizer = new wxFlexGridSizer(2, 0, 0);
    wxASSERT(itemFlexGridSizer);

    itemFlexGridSizer->AddGrowableRow(0);
    itemFlexGridSizer->AddGrowableCol(1);

    m_pTaskPane = new CBOINCTaskCtrl(this, iTaskWindowID, iTaskWindowFlags);
    wxASSERT(m_pTaskPane);

    m_pListPane = new CBOINCListCtrl(this, iListWindowID, iListWindowFlags);
    wxASSERT(m_pListPane);
    
    itemFlexGridSizer->Add(m_pTaskPane, 1, wxGROW|wxALL, 1);
    itemFlexGridSizer->Add(m_pListPane, 1, wxGROW|wxALL, 1);

    SetSizer(itemFlexGridSizer);

    UpdateSelection();

#if USE_NATIVE_LISTCONTROL
    m_pListPane->PushEventHandler(new MyEvtHandler(m_pListPane));
#else
    (m_pListPane->GetMainWin())->PushEventHandler(new MyEvtHandler(m_pListPane));
#endif

    m_iProgressColumn = -1;
    m_iSortColumn = -1;
    m_bReverseSort = false;

    m_SortArrows = new wxImageList(16, 16, true);
    m_SortArrows->Add( wxIcon( sortascending_xpm ) );
    m_SortArrows->Add( wxIcon( sortdescending_xpm ) );
    m_pListPane->SetImageList(m_SortArrows, wxIMAGE_LIST_SMALL);

#if BASEVIEW_STRIPES    
    m_pWhiteBackgroundAttr = new wxListItemAttr(
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW),
        wxNullFont
    );
    m_pGrayBackgroundAttr = new wxListItemAttr(
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),
        wxColour(240, 240, 240),
        wxNullFont
    );
#endif

}


CBOINCBaseView::~CBOINCBaseView() {
    if (m_pListPane) {
#if USE_NATIVE_LISTCONTROL
        m_pListPane->PopEventHandler(true);
#else
        (m_pListPane->GetMainWin())->PopEventHandler(true);
#endif
    }
    if (m_SortArrows) {
        delete m_SortArrows;
    }
    m_arrSelectedKeys1.Clear();
    m_arrSelectedKeys2.Clear();
    m_iSortedIndexes.Clear();

#if BASEVIEW_STRIPES    
    if (m_pWhiteBackgroundAttr) {
        delete m_pWhiteBackgroundAttr;
        m_pWhiteBackgroundAttr = NULL;
    }

    if (m_pGrayBackgroundAttr) {
        delete m_pGrayBackgroundAttr;
        m_pGrayBackgroundAttr = NULL;
    }
#endif
}


// The name of the view.
//   If it has not been defined by the view "Undefined" is returned.
//
wxString& CBOINCBaseView::GetViewName() {
    static wxString strViewName(wxT("Undefined"));
    return strViewName;
}


// The user friendly name of the view.
//   If it has not been defined by the view "Undefined" is returned.
//
wxString& CBOINCBaseView::GetViewDisplayName() {
    static wxString strViewName(wxT("Undefined"));
    return strViewName;
}


// The user friendly icon of the view.
//   If it has not been defined by the view the BOINC icon is returned.
//
const char** CBOINCBaseView::GetViewIcon() {
    wxASSERT(boinc_xpm);
    return boinc_xpm;
}


// The rate at which the view is refreshed.
//   If it has not been defined by the view 1 second is retrned.
//
const int CBOINCBaseView::GetViewRefreshRate() {
    return 1;
}


// Get bit mask of current view(s).
//
const int CBOINCBaseView::GetViewCurrentViewPage() {
    return 0;
}


wxString CBOINCBaseView::GetKeyValue1(int) {
    return wxEmptyString;
}


wxString CBOINCBaseView::GetKeyValue2(int) {
    return wxEmptyString;
}


int CBOINCBaseView::FindRowIndexByKeyValues(wxString&, wxString&) {
    return -1;
}


bool CBOINCBaseView::FireOnSaveState(wxConfigBase* pConfig) {
    return OnSaveState(pConfig);
}


bool CBOINCBaseView::FireOnRestoreState(wxConfigBase* pConfig) {
    return OnRestoreState(pConfig);
}


int CBOINCBaseView::GetListRowCount() {
    wxASSERT(m_pListPane);
    return m_pListPane->GetItemCount();
}


void CBOINCBaseView::FireOnListRender(wxTimerEvent& event) {
    OnListRender(event);
}


void CBOINCBaseView::FireOnListSelected(wxListEvent& event) {
    OnListSelected(event);
}


void CBOINCBaseView::FireOnListDeselected(wxListEvent& event) {
    OnListDeselected(event);
}


wxString CBOINCBaseView::FireOnListGetItemText(long item, long column) const {
    return OnListGetItemText(item, column);
}


int CBOINCBaseView::FireOnListGetItemImage(long item) const {
    return OnListGetItemImage(item);
}


#if BASEVIEW_STRIPES
wxListItemAttr* CBOINCBaseView::FireOnListGetItemAttr(long item) const {
    return OnListGetItemAttr(item);
}


wxListItemAttr* CBOINCBaseView::OnListGetItemAttr(long item) const {

    // If we are using some theme where the default background color isn't
    //   white, then our whole system is boned. Use defaults instead.
    if (wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW) != wxColor(wxT("WHITE"))) return NULL;

    return item % 2 ? m_pGrayBackgroundAttr : m_pWhiteBackgroundAttr;
}
#endif


void CBOINCBaseView::OnListRender(wxTimerEvent& event) {
    if (!m_bProcessingListRenderEvent) {
        m_bProcessingListRenderEvent = true;

        wxASSERT(m_pListPane);

        // Remember the key values of currently selected items
        SaveSelections();
    
        int iDocCount = GetDocCount();
        int iCacheCount = GetCacheCount();
        if (iDocCount != iCacheCount) {
            if (0 >= iDocCount) {
                EmptyCache();
                m_pListPane->DeleteAllItems();
            } else {
                int iIndex = 0;
                int iReturnValue = -1;
                if (iDocCount > iCacheCount) {
                    for (iIndex = 0; iIndex < (iDocCount - iCacheCount); iIndex++) {
                        iReturnValue = AddCacheElement();
                        wxASSERT(!iReturnValue);
                    }
                    wxASSERT(GetDocCount() == GetCacheCount());
                    m_pListPane->SetItemCount(iDocCount);
                    m_bNeedSort = true;
               } else {
                    // The virtual ListCtrl keeps a separate its list of selected rows; 
                    // make sure it does not reference any rows beyond the new last row.
                    // We can ClearSelections() because we called SaveSelections() above.
                    ClearSelections();
                    m_pListPane->SetItemCount(iDocCount);
                    for (iIndex = (iCacheCount - 1); iIndex >= iDocCount; --iIndex) {
                        iReturnValue = RemoveCacheElement();
                        wxASSERT(!iReturnValue);
                    }
                    wxASSERT(GetDocCount() == GetCacheCount());
                    m_pListPane->RefreshItems(0, iDocCount - 1);
                    m_bNeedSort = true;
                }
            }
        }

        if (iDocCount > 0) {
            SynchronizeCache();
            if (iDocCount > 1) {
                if (_EnsureLastItemVisible() && (iDocCount != iCacheCount)) {
                    m_pListPane->EnsureVisible(iDocCount - 1);
                }
            }

            if (m_pListPane->m_bIsSingleSelection) {
                // If no item has been selected yet, select the first item.
#ifdef __WXMSW__
                if ((m_pListPane->GetSelectedItemCount() == 0) &&
                    (m_pListPane->GetItemCount() >= 1)) {

                    long desiredstate = wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED;
                    m_pListPane->SetItemState(0, desiredstate, desiredstate);
                }
#else
                if ((m_pListPane->GetFirstSelected() < 0) &&
                    (m_pListPane->GetItemCount() >= 1)) {
                    m_pListPane->SetItemState(0, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED, 
                                                    wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
                }
#endif
            }
        }
        
        // Find the previously selected items by their key values and reselect them
        RestoreSelections();

        UpdateSelection();
        
        m_bProcessingListRenderEvent = false;
    }

    event.Skip();
}


bool CBOINCBaseView::OnSaveState(wxConfigBase* pConfig) {
    bool bReturnValue = true;

    wxASSERT(pConfig);
    wxASSERT(m_pTaskPane);
    wxASSERT(m_pListPane);

    if (!m_pTaskPane->OnSaveState(pConfig)) {
        bReturnValue = false;
    }

    if (!m_pListPane->OnSaveState(pConfig)) {
        bReturnValue = false;
    }

    return bReturnValue;
}


bool CBOINCBaseView::OnRestoreState(wxConfigBase* pConfig) {
    wxASSERT(pConfig);
    wxASSERT(m_pTaskPane);
    wxASSERT(m_pListPane);

    if (!m_pTaskPane->OnRestoreState(pConfig)) {
        return false;
    }

    if (!m_pListPane->OnRestoreState(pConfig)) {
        return false;
    }
    return true;
}


void CBOINCBaseView::OnListSelected(wxListEvent& event) {
    wxLogTrace(wxT("Function Start/End"), wxT("CBOINCBaseView::OnListSelected - Function Begin"));

    if (!m_bIgnoreUIEvents) {
        m_bForceUpdateSelection = true;
        UpdateSelection();
        event.Skip();
    }

    wxLogTrace(wxT("Function Start/End"), wxT("CBOINCBaseView::OnListSelected - Function End"));
}


void CBOINCBaseView::OnListDeselected(wxListEvent& event) {
    wxLogTrace(wxT("Function Start/End"), wxT("CBOINCBaseView::OnListDeselected - Function Begin"));

    if (!m_bIgnoreUIEvents) {
        m_bForceUpdateSelection = true;
        UpdateSelection();
        event.Skip();
    }

    wxLogTrace(wxT("Function Start/End"), wxT("CBOINCBaseView::OnListDeselected - Function End"));
}


// Work around a bug (feature?) in virtual list control 
//   which does not send deselection events
void CBOINCBaseView::OnCacheHint(wxListEvent& event) {
    static int oldSelectionCount = 0;
    int newSelectionCount = m_pListPane->GetSelectedItemCount();

    if (newSelectionCount < oldSelectionCount) {
        wxListEvent leDeselectedEvent(wxEVT_COMMAND_LIST_ITEM_DESELECTED, m_windowId);
        leDeselectedEvent.SetEventObject(this);
        OnListDeselected(leDeselectedEvent);
    }
    oldSelectionCount = newSelectionCount;
    event.Skip();
}


wxString CBOINCBaseView::OnListGetItemText(long WXUNUSED(item), long WXUNUSED(column)) const {
    return wxString(wxT("Undefined"));
}


int CBOINCBaseView::OnListGetItemImage(long WXUNUSED(item)) const {
    return -1;
}


int CBOINCBaseView::GetDocCount() {
    return 0;
}


wxString CBOINCBaseView::OnDocGetItemImage(long WXUNUSED(item)) const {
    return wxString(wxT("Undefined"));
}


wxString CBOINCBaseView::OnDocGetItemAttr(long WXUNUSED(item)) const {
    return wxString(wxT("Undefined"));
}


int CBOINCBaseView::AddCacheElement() {
    return -1;
}

    
int CBOINCBaseView::EmptyCache() {
    return -1;
}


int CBOINCBaseView::GetCacheCount() {
    return -1;
}


int CBOINCBaseView::RemoveCacheElement() {
    return -1;
}


int CBOINCBaseView::SynchronizeCache() {
    int         iRowIndex        = 0;
    int         iRowTotal        = 0;
    int         iColumnIndex     = 0;
    int         iColumnTotal     = 0;
    bool        bNeedRefreshData = false;

    iRowTotal = GetDocCount();
    iColumnTotal = m_pListPane->GetColumnCount();
    
    for (iRowIndex = 0; iRowIndex < iRowTotal; iRowIndex++) {
        bNeedRefreshData = false;

        for (iColumnIndex = 0; iColumnIndex < iColumnTotal; iColumnIndex++) {
            if (SynchronizeCacheItem(iRowIndex, iColumnIndex)) {
#ifdef __WXMAC__
                bNeedRefreshData = true;
#else
                // To reduce flicker, refresh only changed columns
                m_pListPane->RefreshCell(iRowIndex, iColumnIndex);
#endif
                if (iColumnIndex == m_iSortColumn) {
                    m_bNeedSort = true;
                }
            }
        }

        // Mac is double-buffered to avoid flicker, so this is more efficient
        if (bNeedRefreshData) {
            m_pListPane->RefreshItem(iRowIndex);
        }
    }

    if (m_bNeedSort) {
        sortData();     // Will mark moved items as needing refresh
        m_bNeedSort = false;
    }
    
    return 0;
}


bool CBOINCBaseView::SynchronizeCacheItem(wxInt32 WXUNUSED(iRowIndex), wxInt32 WXUNUSED(iColumnIndex)) {
    return false;
}


void CBOINCBaseView::OnColClick(wxListEvent& event) {
    wxListItem      item;
    int             newSortColumn = event.GetColumn();
    wxArrayInt      selections;
    int             i, j, m;

    item.SetMask(wxLIST_MASK_IMAGE);
    if (newSortColumn == m_iSortColumn) {
        m_bReverseSort = !m_bReverseSort;
    } else {
        // Remove sort arrow from old sort column
        if (m_iSortColumn >= 0) {
            item.SetImage(-1);
            m_pListPane->SetColumn(m_iSortColumn, item);
        }
        m_iSortColumn = newSortColumn;
        m_bReverseSort = false;
    }
    
    item.SetImage(m_bReverseSort ? 0 : 1);
    m_pListPane->SetColumn(newSortColumn, item);
    
    Freeze();   // To reduce flicker
    // Remember which cache elements are selected and deselect them
    m_bIgnoreUIEvents = true;
    i = -1;
    while (1) {
        i = m_pListPane->GetNextSelected(i);
        if (i < 0) break;
        selections.Add(m_iSortedIndexes[i]);
        m_pListPane->SelectRow(i, false);
    }
    
    sortData();

    // Reselect previously selected cache elements in the sorted list 
    m = (int)selections.GetCount();
    for (i=0; i<m; i++) {
        if (selections[i] >= 0) {
            j = m_iSortedIndexes.Index(selections[i]);
            m_pListPane->SelectRow(j, true);
        }
    }
    m_bIgnoreUIEvents = false;

    Thaw();
}


void CBOINCBaseView::InitSort() {
    wxListItem      item;

    if (m_iSortColumn < 0) return;
    item.SetMask(wxLIST_MASK_IMAGE);
    item.SetImage(m_bReverseSort ? 0 : 1);
    m_pListPane->SetColumn(m_iSortColumn, item);
    Freeze();   // To reduce flicker
    sortData();
    Thaw();
}


void CBOINCBaseView::sortData() {
    if (m_iSortColumn < 0) return;
    
    wxArrayInt oldSortedIndexes(m_iSortedIndexes);
    int i, n = (int)m_iSortedIndexes.GetCount();
    
    std::stable_sort(m_iSortedIndexes.begin(), m_iSortedIndexes.end(), m_funcSortCompare);
    
    // Refresh rows which have moved
    for (i=0; i<n; i++) {
        if (m_iSortedIndexes[i] != oldSortedIndexes[i]) {
            m_pListPane->RefreshItem(i);
         }
    }
}


void CBOINCBaseView::EmptyTasks() {
    unsigned int i;
    unsigned int j;
    for (i=0; i<m_TaskGroups.size(); i++) {
        for (j=0; j<m_TaskGroups[i]->m_Tasks.size(); j++) {
            delete m_TaskGroups[i]->m_Tasks[j];
        }
        m_TaskGroups[i]->m_Tasks.clear();
        delete m_TaskGroups[i];
    }
    m_TaskGroups.clear();
}

    
void CBOINCBaseView::ClearSavedSelections() {
    m_arrSelectedKeys1.Clear();
    m_arrSelectedKeys2.Clear();
}


// Save the key values of the currently selected rows for later restore
void CBOINCBaseView::SaveSelections() {
    if (!_IsSelectionManagementNeeded()) {
        return;
    }

    m_arrSelectedKeys1.Clear();
    m_arrSelectedKeys2.Clear();
    m_bIgnoreUIEvents = true;
    int i = -1;
    while (1) {
        i = m_pListPane->GetNextSelected(i);
        if (i < 0) break;
        m_arrSelectedKeys1.Add(GetKeyValue1(i));
        m_arrSelectedKeys2.Add(GetKeyValue2(i));
    }
    m_bIgnoreUIEvents = false;
}

// Select all rows with formerly selected data based on key values
//
// Each RPC may add (insert) or delete items from the list, so 
// the selected row numbers may now point to different data.  
// We called SaveSelections() before updating the underlying 
// data; this routine finds the data corresponding to each 
// previous selection and makes any adjustments to ensure that 
// the rows containing the originally selected data are selected.
void CBOINCBaseView::RestoreSelections() {
    if (!_IsSelectionManagementNeeded()) {
        return;
    }

    // To minimize flicker, this method selects or deselects only 
    // those rows which actually need their selection status changed.
    // First, get a list of which rows should be selected
    int i, j, m = 0, newCount = 0, oldCount = (int)m_arrSelectedKeys1.size();
    bool found;
    wxArrayInt arrSelRows;
    for(i=0; i< oldCount; ++i) {
		int index = FindRowIndexByKeyValues(m_arrSelectedKeys1[i], m_arrSelectedKeys2[i]);
		if(index >= 0) {
            arrSelRows.Add(index);
		}
	}
    newCount = (int)arrSelRows.GetCount();
    
    // Step through the currently selected row numbers and for each one determine 
    // whether it should remain selected.
    m_bIgnoreUIEvents = true;
    i = -1;
    while (1) {
        found = false;
        i = m_pListPane->GetNextSelected(i);
        if (i < 0) break;
        m = (int)arrSelRows.GetCount();
        for (j=0; j<m; ++j) {
            if (arrSelRows[j] == i) {
                arrSelRows.RemoveAt(j); // We have handled this one so remove from list
                found = true;           // Already selected, so no change needed
                break;
            }
        }
        if (!found) {
            m_pListPane->SelectRow(i, false);  // This row should no longer be selected
        }
    }
    
    // Now select those rows which were not previously selected but should now be
    m = (int)arrSelRows.GetCount();
    for (j=0; j<m; ++j) {
        m_pListPane->SelectRow(arrSelRows[j], true);
    }
    m_bIgnoreUIEvents = false;
    
    if (oldCount != newCount) {
        m_bForceUpdateSelection = true; // OnListRender() will call UpdateSelection()
    }
}


void CBOINCBaseView::ClearSelections() {
    if (!m_pListPane) return;
    
    m_bIgnoreUIEvents = true;
    int i = -1;
    while (1) {
        i = m_pListPane->GetNextSelected(i);
        if (i < 0) break;
        m_pListPane->SelectRow(i, false);
    }
    m_bIgnoreUIEvents = false;
}


void CBOINCBaseView::PreUpdateSelection(){
}


void CBOINCBaseView::UpdateSelection(){
}


void CBOINCBaseView::PostUpdateSelection(){
    wxASSERT(m_pTaskPane);
    m_pTaskPane->UpdateControls();
    Layout();
}


void CBOINCBaseView::UpdateWebsiteSelection(long lControlGroup, PROJECT* project){
    unsigned int        i;
    CTaskItemGroup*     pGroup = NULL;
    CTaskItem*          pItem = NULL;

    wxASSERT(m_pTaskPane);
    wxASSERT(m_pListPane);

    if (m_bForceUpdateSelection) {

        // Update the websites list
        //
        if (m_TaskGroups.size() > 1) {

            // Delete task group, objects, and controls.
            pGroup = m_TaskGroups[lControlGroup];

            m_pTaskPane->DeleteTaskGroupAndTasks(pGroup);
            for (i=0; i<pGroup->m_Tasks.size(); i++) {
                delete pGroup->m_Tasks[i];
            }
            pGroup->m_Tasks.clear();
            delete pGroup;

            pGroup = NULL;

            m_TaskGroups.erase( m_TaskGroups.begin() + 1 );
        }

        // If something is selected create the tasks and controls
        //
        if (m_pListPane->GetSelectedItemCount()) {
            if (project) {
                // Create the web sites task group
                pGroup = new CTaskItemGroup( _("Project web pages") );
                m_TaskGroups.push_back( pGroup );

                // Default project url
                pItem = new CTaskItem(
                    wxString("Home page", wxConvUTF8), 
                    wxString(project->project_name.c_str(), wxConvUTF8) + wxT(" web site"), 
                    wxString(project->master_url, wxConvUTF8),
                    ID_TASK_PROJECT_WEB_PROJDEF_MIN
                );
                pGroup->m_Tasks.push_back(pItem);


                // Project defined urls
                for (i=0;(i<project->gui_urls.size())&&(i<=ID_TASK_PROJECT_WEB_PROJDEF_MAX);i++) {
                    pItem = new CTaskItem(
                        wxGetTranslation(wxString(project->gui_urls[i].name.c_str(), wxConvUTF8)),
                        wxGetTranslation(wxString(project->gui_urls[i].description.c_str(), wxConvUTF8)),
                        wxString(project->gui_urls[i].url.c_str(), wxConvUTF8),
                        ID_TASK_PROJECT_WEB_PROJDEF_MIN + 1 + i
                    );
                    pGroup->m_Tasks.push_back(pItem);
                }
            }
        }

        m_bForceUpdateSelection = false;
    }
}


// Make sure task pane background is properly erased
void CBOINCBaseView::RefreshTaskPane() {
    if (m_pTaskPane) {
        m_pTaskPane->Refresh(true);
    }
}


bool CBOINCBaseView::_IsSelectionManagementNeeded() {
    return IsSelectionManagementNeeded();
}


bool CBOINCBaseView::IsSelectionManagementNeeded() {
    return false;
}


bool CBOINCBaseView::_EnsureLastItemVisible() {
    return EnsureLastItemVisible();
}


bool CBOINCBaseView::EnsureLastItemVisible() {
    return false;
}


double CBOINCBaseView::GetProgressValue(long) {
    return 0.0;
}


wxString CBOINCBaseView::GetProgressText( long ) {
    return wxEmptyString;
}


void CBOINCBaseView::append_to_status(wxString& existing, const wxChar* additional) {
    if (existing.size() == 0) {
        existing = additional;
    } else {
        existing = existing + wxT(", ") + additional;
    }
}


// HTML Entity Conversion:
// http://www.webreference.com/html/reference/character/
// Completed: The ISO Latin 1 Character Set
//
wxString CBOINCBaseView::HtmlEntityEncode(wxString strRaw) {
	wxString strEncodedHtml(strRaw);

#ifdef __WXMSW__
    strEncodedHtml.Replace(wxT("&"),  wxT("&amp;"),    true);
    strEncodedHtml.Replace(wxT("\""), wxT("&quot;"),   true);
    strEncodedHtml.Replace(wxT("<"),  wxT("&lt;"),     true);
    strEncodedHtml.Replace(wxT(">"),  wxT("&gt;"),     true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&sbquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&fnof;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&bdquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&hellip;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&dagger;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Dagger;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Scaron;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&OElig;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&lsquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&rsquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ldquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&rdquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&bull;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ndash;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&mdash;"),  true);
    strEncodedHtml.Replace(wxT("��~"),  wxT("&tilde;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&trade;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&scaron;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&oelig;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Yuml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&iexcl;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&cent;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&pound;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&curren;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&yen;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&brvbar;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&sect;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&uml;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&copy;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ordf;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&laquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&not;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&reg;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&macr;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&deg;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&plusmn;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&sup2;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&sup3;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&acute;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&micro;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&para;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&middot;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&cedil;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&sup1;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ordm;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&raquo;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&frac14;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&frac12;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&frac34;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&iquest;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Agrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Aacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Acirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Atilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Auml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Aring;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&AElig;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ccedil;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Egrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Eacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ecirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Euml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Igrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Iacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Icirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Iuml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ETH;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ntilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ograve;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Oacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ocirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Otilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ouml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&times;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Oslash;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ugrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Uacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Ucirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Uuml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&Yacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&THORN;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&szlig;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&agrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&aacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&acirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&atilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&auml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&aring;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&aelig;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ccedil;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&egrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&eacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ecirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&euml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&igrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&iacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&icirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&iuml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&eth;"),    true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ntilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ograve;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&oacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ocirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&otilde;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ouml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&divide;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&oslash;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ugrave;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&uacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&ucirc;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&uuml;"),   true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&yacute;"), true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&thorn;"),  true);
    strEncodedHtml.Replace(wxT("�"),  wxT("&yuml;"),   true);
#endif

    return strEncodedHtml;
}

wxString CBOINCBaseView::HtmlEntityDecode(wxString strRaw) {
	wxString strDecodedHtml(strRaw);

    if (0 <= strDecodedHtml.Find(wxT("&"))) {
#ifdef __WXMSW__
        strDecodedHtml.Replace(wxT("&amp;"),    wxT("&"),  true);
        strDecodedHtml.Replace(wxT("&quot;"),   wxT("\""), true);
        strDecodedHtml.Replace(wxT("&lt;"),     wxT("<"),  true);
        strDecodedHtml.Replace(wxT("&gt;"),     wxT(">"),  true);
        strDecodedHtml.Replace(wxT("&sbquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&fnof;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&bdquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&hellip;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&dagger;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Dagger;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Scaron;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&OElig;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&lsquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&rsquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ldquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&rdquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&bull;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ndash;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&mdash;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&tilde;"),  wxT("��~"),  true);
        strDecodedHtml.Replace(wxT("&trade;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&scaron;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&oelig;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Yuml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&iexcl;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&cent;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&pound;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&curren;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&yen;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&brvbar;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&sect;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&uml;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&copy;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ordf;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&laquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&not;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&reg;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&macr;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&deg;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&plusmn;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&sup2;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&sup3;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&acute;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&micro;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&para;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&middot;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&cedil;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&sup1;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ordm;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&raquo;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&frac14;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&frac12;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&frac34;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&iquest;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Agrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Aacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Acirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Atilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Auml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Aring;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&AElig;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ccedil;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Egrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Eacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ecirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Euml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Igrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Iacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Icirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Iuml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ETH;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ntilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ograve;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Oacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ocirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Otilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ouml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&times;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Oslash;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ugrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Uacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Ucirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Uuml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&Yacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&THORN;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&szlig;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&agrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&aacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&acirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&atilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&auml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&aring;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&aelig;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ccedil;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&egrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&eacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ecirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&euml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&igrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&iacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&icirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&iuml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&eth;"),    wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ntilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ograve;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&oacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ocirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&otilde;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ouml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&divide;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&oslash;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ugrave;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&uacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&ucirc;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&uuml;"),   wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&yacute;"), wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&thorn;"),  wxT("�"),  true);
        strDecodedHtml.Replace(wxT("&yuml;"),   wxT("�"),  true);
#endif
    }

	return strDecodedHtml;
}

