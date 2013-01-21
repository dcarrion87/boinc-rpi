/////////////////////////////////////////////////////////////////////////////
// Name:        fs_mem.cpp
// Purpose:     in-memory file system
// Author:      Vaclav Slavik
// RCS-ID:      $Id: fs_mem.cpp 46522 2007-06-18 18:37:40Z VS $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "BOINCInternetFSHandler.h"
#endif

#include "stdwx.h"
#include "BOINCInternetFSHandler.h"
#include "BOINCGUIApp.h"
#include "MainDocument.h"

class MemFSHashObj : public wxObject
{
public:
    MemFSHashObj(wxInputStream* stream, const wxString& mime)
    {
        wxMemoryOutputStream out;
        stream->Read(out);
        m_Len = out.GetSize();
        m_Data = new char[m_Len];
        out.CopyTo(m_Data, m_Len);
        m_MimeType = mime;
        m_Time = wxDateTime::Now();
    }

    virtual ~MemFSHashObj()
    {
        delete[] m_Data;
    }

    char *m_Data;
    size_t m_Len;
    wxString m_MimeType;
    wxDateTime m_Time;

    DECLARE_NO_COPY_CLASS(MemFSHashObj)
};


wxHashTable *CBOINCInternetFSHandler::m_Hash = NULL;

static bool b_ShuttingDown = false;

#ifdef __WXMSW__
// *** code adapted from src/msw/urlmsw.cpp (wxWidgets 2.8.10)

#ifdef __VISUALC__  // be conservative about this pragma
    // tell the linker to include wininet.lib automatically
    #pragma comment(lib, "wininet.lib")
#endif

#include "wx/url.h"

#include <string.h>
#include <ctype.h>
#include <wininet.h>

// this class needn't be exported
class wxWinINetURL
{
public:
    wxInputStream *GetInputStream(wxURL *owner);

protected:
    // return the WinINet session handle
    static HINTERNET GetSessionHandle();
};

////static bool lastReadHadEOF = false;
static bool operationEnded = false;
static DWORD lastInternetStatus;
static LPVOID lastlpvStatusInformation;
static DWORD lastStatusInfo;
static DWORD lastStatusInfoLength;

// Callback for InternetOpenURL() and InternetReadFileEx()
static void CALLBACK BOINCInternetStatusCallback(
                    HINTERNET,
                    DWORD_PTR,
                    DWORD dwInternetStatus,
                    LPVOID lpvStatusInformation,
                    DWORD dwStatusInformationLength
            )
{
    lastInternetStatus = dwInternetStatus;
    lastlpvStatusInformation = lpvStatusInformation;
    lastStatusInfoLength = dwStatusInformationLength;
    if (lastStatusInfoLength == sizeof(DWORD)) {
        lastStatusInfo = *(DWORD*)lpvStatusInformation;
    }
    switch (dwInternetStatus) {
    case INTERNET_STATUS_REQUEST_COMPLETE:
        operationEnded = true;
        break;
    case INTERNET_STATUS_STATE_CHANGE:
        if (lastStatusInfo & (INTERNET_STATE_DISCONNECTED | INTERNET_STATE_DISCONNECTED_BY_USER)) {
            operationEnded = true;
        }
        break;
    }
}


HINTERNET wxWinINetURL::GetSessionHandle()
{
    // this struct ensures that the session is opened when the
    // first call to GetSessionHandle is made
    // it also ensures that the session is closed when the program
    // terminates
    static struct INetSession
    {
        INetSession()
        {
            INetOpenSession();
        }

        ~INetSession()
        {
            INetCloseSession();
        }

        void INetOpenSession() {
            DWORD rc = InternetAttemptConnect(0);

            m_handle = InternetOpen
                       (
                        wxVERSION_STRING,
                        INTERNET_OPEN_TYPE_PRECONFIG,
                        NULL,
                        NULL,
                        INTERNET_FLAG_ASYNC |
                            (rc == ERROR_SUCCESS ? 0 : INTERNET_FLAG_OFFLINE)
                       );

            if (m_handle) {
                InternetSetStatusCallback(m_handle, BOINCInternetStatusCallback);
            }
        }
        
        void INetCloseSession() {
            InternetSetStatusCallback(NULL, BOINCInternetStatusCallback);
            if (m_handle) {
                InternetCloseHandle(m_handle);
                m_handle = NULL;
            }
        }
    
        HINTERNET m_handle;
    } session;
    
     CMainDocument* pDoc      = wxGetApp().GetDocument();

    wxASSERT(pDoc);

    if (b_ShuttingDown || (!pDoc->IsConnected())) {
        session.INetCloseSession();
        return 0;
    }

    if (!session.m_handle) {
        session.INetOpenSession();
    }
    return session.m_handle;
}
\


// this class needn't be exported
class /*WXDLLIMPEXP_NET */ wxWinINetInputStream : public wxInputStream
{
public:
    wxWinINetInputStream(HINTERNET hFile=0);
    virtual ~wxWinINetInputStream();

    void Attach(HINTERNET hFile);

    wxFileOffset SeekI( wxFileOffset WXUNUSED(pos), wxSeekMode WXUNUSED(mode) )
        { return -1; }
    wxFileOffset TellI() const
        { return -1; }
    size_t GetSize() const;

protected:
    void SetError(wxStreamError err) { m_lasterror=err; }
    HINTERNET m_hFile;
    size_t OnSysRead(void *buffer, size_t bufsize);

    DECLARE_NO_COPY_CLASS(wxWinINetInputStream)
};


size_t wxWinINetInputStream::GetSize() const
{
    DWORD contentLength = 0;
    DWORD dwSize = sizeof(contentLength);
    DWORD index = 0;

    if (!m_hFile) {
        return 0;
    }
    if ( HttpQueryInfo( m_hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwSize, &index) )
        return contentLength;
    else
        return 0;
}


size_t wxWinINetInputStream::OnSysRead(void *buffer, size_t bufsize)
{
    DWORD bytesread = 0;
    DWORD lError = ERROR_SUCCESS;
    INTERNET_BUFFERS bufs;
    BOOL complete = false;
    CMainDocument* pDoc      = wxGetApp().GetDocument();

    wxASSERT(pDoc);

    if (b_ShuttingDown || (!pDoc->IsConnected())) {
        return 0;
    }

    if (m_hFile) {
        operationEnded = false;
        memset(&bufs, 0, sizeof(bufs));
        bufs.dwStructSize = sizeof(INTERNET_BUFFERS);
        bufs.Next = NULL;
        bufs.lpvBuffer = buffer;
        bufs.dwBufferLength = (DWORD)bufsize;

        lastInternetStatus = 0;
        complete = InternetReadFileEx(m_hFile, &bufs,  IRF_ASYNC | IRF_USE_CONTEXT, 2);
        
        if (!complete) {
            lError = ::GetLastError();
            if ((lError == WSAEWOULDBLOCK) || (lError == ERROR_IO_PENDING)){
                while (!operationEnded) {
                    if (b_ShuttingDown || (!pDoc->IsConnected())) {
//                    if (b_ShuttingDown) {
                        SetError(wxSTREAM_EOF);
                        return 0;
                    }
                    wxGetApp().Yield(true);
                }
            }
        }

        lError = ::GetLastError();
        bytesread = bufs.dwBufferLength;
        complete = (lastInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE);
    }

    if (!complete) {
        if ( lError != ERROR_SUCCESS )
            SetError(wxSTREAM_READ_ERROR);

#if 0       // Possibly useful for debugging
        DWORD iError, bLength = 0;
        InternetGetLastResponseInfo(&iError, NULL, &bLength);
        if ( bLength > 0 )
        {
            wxString errorString;
            InternetGetLastResponseInfo
            (
                &iError,
                wxStringBuffer(errorString, bLength),
                &bLength
            );

            wxLogError(wxT("Read failed with error %d: %s"),
                    iError, errorString.c_str());
        }
#endif
    }

    if ( bytesread == 0 )
    {
        SetError(wxSTREAM_EOF);
    } else {
        // Apparently buffer has not always received all the data even
        // when callback returns INTERNET_STATUS_REQUEST_COMPLETE.
        // This extra delay seems to fix that.  Why????
        SleepEx(10, TRUE);
    }

    return bytesread;
}


wxWinINetInputStream::wxWinINetInputStream(HINTERNET hFile)
                    : m_hFile(hFile)
{
}


void wxWinINetInputStream::Attach(HINTERNET newHFile)
{
    wxCHECK_RET(m_hFile==NULL,
        wxT("cannot attach new stream when stream already exists"));
    m_hFile=newHFile;
    SetError(m_hFile!=NULL ? wxSTREAM_NO_ERROR : wxSTREAM_READ_ERROR);
}


wxWinINetInputStream::~wxWinINetInputStream()
{
    if ( m_hFile )
    {
        InternetCloseHandle(m_hFile);
        m_hFile=0;
    }
}


wxInputStream *wxWinINetURL::GetInputStream(wxURL *owner)
{
    DWORD service;
    CMainDocument* pDoc      = wxGetApp().GetDocument();

    wxASSERT(pDoc);

    if (b_ShuttingDown || (!pDoc->IsConnected())) {
        GetSessionHandle(); // Closes the session
        return 0;
    }
    
    if ( owner->GetScheme() == wxT("http") )
    {
        service = INTERNET_SERVICE_HTTP;
    }
    else if ( owner->GetScheme() == wxT("ftp") )
    {
        service = INTERNET_SERVICE_FTP;
    }
    else
    {
        // unknown protocol. Let wxURL try another method.
        return 0;
    }
    
    wxWinINetInputStream *newStream = new wxWinINetInputStream;
    
    operationEnded = false;

    HINTERNET newStreamHandle = InternetOpenUrl
                                (
                                    GetSessionHandle(),
                                    owner->GetURL(),
                                    NULL,
                                    0,
                                    INTERNET_FLAG_KEEP_CONNECTION |
                                    INTERNET_FLAG_PASSIVE,
                                    1
                                );
                              
    while (!operationEnded) {
        if (b_ShuttingDown || (!pDoc->IsConnected())) {
            GetSessionHandle(); // Closes the session
            if (newStreamHandle) {
                delete newStreamHandle;
                newStreamHandle = NULL;
            }
            if (newStream) {
                delete newStream;
                newStream = NULL;
            }
            return 0;
        }
        wxGetApp().Yield(true);
    }

    if ((lastInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE) &&
            (lastStatusInfoLength >= sizeof(HINTERNET)) &&
            (!b_ShuttingDown)
        ) {
            INTERNET_ASYNC_RESULT* res = (INTERNET_ASYNC_RESULT*)lastlpvStatusInformation;
            newStreamHandle = (HINTERNET)(res->dwResult);
    }

    newStream->Attach(newStreamHandle);

    InternetSetStatusCallback(newStreamHandle, BOINCInternetStatusCallback);    // IS THIS NEEDED???

    return newStream;
}

// *** End of code adapted from src/msw/urlmsw.cpp (wxWidgets 2.8.10)
#endif // __WXMSW__


CBOINCInternetFSHandler::CBOINCInternetFSHandler() : wxFileSystemHandler()
{
    m_InputStream = NULL;
    b_ShuttingDown = false;
    
    if (!m_Hash)
    {
        m_Hash = new wxHashTable(wxKEY_STRING);
    }
}


CBOINCInternetFSHandler::~CBOINCInternetFSHandler()
{
    // as only one copy of FS handler is supposed to exist, we may silently
    // delete static data here. (There is no way how to remove FS handler from
    // wxFileSystem other than releasing _all_ handlers.)
    if (m_Hash)
    {
        WX_CLEAR_HASH_TABLE(*m_Hash);
        delete m_Hash;
        m_Hash = NULL;
    }
}


static wxString StripProtocolAnchor(const wxString& location)
{
    wxString myloc(location.BeforeLast(wxT('#')));
    if (myloc.empty()) myloc = location.AfterFirst(wxT(':'));
    else myloc = myloc.AfterFirst(wxT(':'));

    // fix malformed url:
    if (!myloc.Left(2).IsSameAs(wxT("//")))
    {
        if (myloc.GetChar(0) != wxT('/')) myloc = wxT("//") + myloc;
        else myloc = wxT("/") + myloc;
    }
    if (myloc.Mid(2).Find(wxT('/')) == wxNOT_FOUND) myloc << wxT('/');

    return myloc;
}


bool CBOINCInternetFSHandler::CanOpen(const wxString& location)
{
    if (b_ShuttingDown) return false;

    wxString p = GetProtocol(location);
    if ((p == wxT("http")) || (p == wxT("ftp")))
    {
        wxURL url(p + wxT(":") + StripProtocolAnchor(location));
        return (url.GetError() == wxURL_NOERR);
    }
    return false;
}


wxFSFile* CBOINCInternetFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs), const wxString& strLocation)
{
    wxString strMIME;

    if (b_ShuttingDown) return NULL;

    if (m_Hash)
    {
        MemFSHashObj* obj = (MemFSHashObj*)m_Hash->Get(strLocation);
        if (obj == NULL)
        {
            wxString right = GetProtocol(strLocation) + wxT(":") + StripProtocolAnchor(strLocation);

            wxURL url(right);
            if (url.GetError() == wxURL_NOERR)
            {
                
#ifdef __WXMSW__
                wxWinINetURL * winURL = new wxWinINetURL;
                wxInputStream* m_InputStream = winURL->GetInputStream(&url);
#else
                wxInputStream* m_InputStream = url.GetInputStream();
#endif
                strMIME = url.GetProtocol().GetContentType();
                if (strMIME == wxEmptyString) {
                    strMIME = GetMimeTypeFromExt(strLocation);
                }

                if (m_InputStream)
                {
                    obj = new MemFSHashObj(m_InputStream, strMIME);
                    delete m_InputStream;
                    m_InputStream = NULL;
 
                    // If we couldn't read image, then return NULL so 
                    // image tag handler displays "broken image" bitmap
                    if (obj->m_Len == 0) {
                        delete obj;
                        return NULL;
                   }
 
                    m_Hash->Put(strLocation, obj);
                    
                    return new wxFSFile (
                        new wxMemoryInputStream(obj->m_Data, obj->m_Len),
                        strLocation,
                        strMIME,
                        GetAnchor(strLocation),
                        obj->m_Time
                    );
                }
            }
        }
        else
        {
            strMIME = obj->m_MimeType;
            if ( strMIME.empty() ) {
                strMIME = GetMimeTypeFromExt(strLocation);
            }

            return new wxFSFile (
                new wxMemoryInputStream(obj->m_Data, obj->m_Len),
                strLocation,
                strMIME,
                GetAnchor(strLocation),
                obj->m_Time
            );
        }
    }
    
    return NULL;
}


bool CBOINCInternetFSHandler::CheckHash(const wxString& strLocation)
{
    if (m_Hash->Get(strLocation) != NULL)
        return false;
    else
        return true;
}


void CBOINCInternetFSHandler::ShutDown() {
    b_ShuttingDown = true;
#ifdef __WXMSW__
    if (m_InputStream) {
        delete m_InputStream;
        m_InputStream = NULL;
    }
#endif
}
