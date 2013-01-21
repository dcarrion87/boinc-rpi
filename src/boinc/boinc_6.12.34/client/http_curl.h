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

// HTTP_OP represents an HTTP operation.
// There are variants for GET and POST,
// and for the data source/sink (see below).

// We use libcurl: http://curl.haxx.se/libcurl

#ifndef _HTTP_CURL_
#define _HTTP_CURL_

#include <curl/curl.h>

#include "network.h"
#include "proxy_info.h"

extern int curl_init();
extern int curl_cleanup();

#define HTTP_OP_NONE    0
#define HTTP_OP_GET     1
    // data sink is a file (used for file download)
#define HTTP_OP_POST    2
    // data source and sink are files (used for scheduler op)
#define HTTP_OP_HEAD    4
    // no data (used for file upload)
#define HTTP_OP_POST2   5
    // a POST operation where the request comes from a combination
    // of a string and a file w/offset,
    // and the reply goes into a memory buffer.
    // Used for file upload

#define HTTP_STATE_IDLE             0
#define HTTP_STATE_CONNECTING       1
#define HTTP_STATE_DONE             2

class HTTP_OP {
public:
    HTTP_OP();
    ~HTTP_OP();

    PROXY_INFO pi;

	char m_url[256];  
	char m_curl_ca_bundle_location[256];
        // string needed for ssl support
	char m_curl_user_credentials[128];
        // string needed for proxy username/password

    int content_length;
    double file_offset;
    unsigned int trace_id;
    char request_header[4096];

    FILE* fileIn;
	FILE* fileOut;
        // CMC need an output file for POST responses
	CURL* curlEasy;
        // the "easy curl" handle for this net_xfer request
	struct curl_slist *pcurlList;
        // curl slist for http headers
	struct curl_httppost *pcurlFormStart;
        // a pointer to a form item for POST
	struct curl_httppost *pcurlFormEnd;
        // a pointer to a form item for POST
	unsigned char* pByte;
        // pointer to bytes for reading via libcurl_read function

	long lSeek;
        // offset within the file or memory buffer we're reading,
    char infile[256];
    char outfile[256];
    char error_msg[256];
        // put Curl error message here
	bool bTempOutfile;
        // CMC -- flag that outfile is really a tempfile we should delete
    char* req1;
    int req1_len;
	bool bSentHeader;
        // CMC -- a flag that I already sent the header
	CURLcode CurlResult;
        // CMC -- send up curl result code

    bool want_download;     // at most one should be true
    bool want_upload;
    long connect_error;
        // errno from connect() (not used for anything)
	long response;
        // HTTP status code from server
        // the above two MUST be long (not int)
        // otherwise breaks on 64-bit machines
    double start_time;
    double bytes_xferred;
        // Uncompressed bytes transferred.
        // In the case of "post2" this includes only the file part
        // In the case of restartable ops (file upload/download)
        // this includes previous count (i.e. file offset)
	double start_bytes_xferred;
        // bytes_xferred at the start of this operation;
        // used to compute transfer speed
    double xfer_speed;
        // tranfer rate based on elapsed time and bytes_xferred
        // (hence doesn't reflect compression; used only for GUI)
	int http_op_state;      // values above
    int http_op_type;
        // HTTP_OP_* (see above)
    int http_op_retval;
        // Either:
        // 0
        // ERR_GETHOSTBYNAME (if no such host)
        // ERR_CONNECT (if server down)
        // ERR_FILE_NOT_FOUND (if 404)
        // ERR_HTTP_ERROR (other failures)
    bool is_background;
        // this operation shouldn't trigger a reference-site check
        // and a user alert if it fails.
        // For example: notice RSS feed fetches

    void reset();
    void init();
    int get_ip_addr(int &ip_addr);
    void close_socket();
    void close_file();
    void update_speed();
    void set_speed_limit(bool is_upload, double bytes_sec);
    void handle_messages(CURLMsg*);

	//int init_head(const char* url);
    int init_get(const char* url, const char* outfile, bool del_old_file, double offset=0);
    int init_post(const char* url, const char* infile, const char* outfile);
    int init_post2(
        const char* url,
        char* req1,     // first part of request.  ALSO USED FOR REPLY
        int req1_len,
        const char* infile, double offset     // infile is NULL if no file sent
    );
    bool http_op_done();
	void setup_proxy_session(bool no_proxy);
	bool no_proxy_for_url(const char* url);
	bool is_active() {
		return curlEasy!=NULL;
	}

private:
	// take an init_get/post/post2 and turns it into a libcurl request
    //
	int libcurl_exec(const char* url, const char* in, const char* out, 
		double offset, bool bPost
    );
};

// represents a set of HTTP requests in progress

class HTTP_OP_SET {
    std::vector<HTTP_OP*> http_ops;
public:
    HTTP_OP_SET();
    void insert(HTTP_OP*);
    int remove(HTTP_OP*);
    int nops();

    double bytes_up, bytes_down;
        // total bytes transferred

	void get_fdset(FDSET_GROUP&);
    void got_select(FDSET_GROUP&, double);
    HTTP_OP* lookup_curl(CURL* pcurl);
        // lookup by easycurl handle
    void cleanup_temp_files();

};

extern char* get_user_agent_string();

#endif //__HTTP_CURL_
