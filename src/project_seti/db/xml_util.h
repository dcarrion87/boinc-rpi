// $Id: xml_util.h,v 1.8.2.6 2007/05/31 22:03:31 korpela Exp $
// The contents of this file are subject to the BOINC Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://boinc.berkeley.edu/license_1.0.txt
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License.
//
// The Original Code is the Berkeley Open Infrastructure for Network Computing.
//
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved.
//
// Contributor(s):
//
//  Additional routines to help maintain XML compliance.
//
// Revision History:
// $Log: xml_util.h,v $
// Revision 1.8.2.6  2007/05/31 22:03:31  korpela
// *** empty log message ***
//
// Revision 1.8.2.5  2007/03/09 00:21:15  vonkorff
// Fixed missing trailing null in base64 encoding.
//
// Revision 1.8.2.4  2006/12/14 22:26:30  korpela
// *** empty log message ***
//
// Revision 1.8.2.3  2005/12/01 00:01:01  korpela
// Changed to an Intel compile of fftw3.dll.  Hopefully this will fix the problem
// with crashes on Pentium-MMX and earlier.
//
// Added a Dev-C++ project and modified source files to allow seti_boinc to compile
// under MinGW.
//
// Revision 1.8.2.2  2005/10/17 22:33:33  korpela
// Continuing previous fix
//
// Revision 1.8.2.1  2005/10/17 22:14:49  korpela
// Fixed bugs with XML formatting and CSV vector input.  The XML formatting bug
// fixes were submitted by Tetsuji "Maverick" Rai.  The CSV input problem was due
// to my misunderstanding of the istream::operator void *().  I had thought the
// operator returned NULL if the past operation had failed.  Instead it returns
// NULL if the NEXT operation will fail.
//
// Revision 1.8  2004/12/07 23:42:23  korpela
// Added undef of min and max in case they are defined in a previously included
// header file.
//
// Revision 1.7  2004/11/18 19:17:04  korpela
// Fixed base64 and base85 decoding.
//
//
// Revision 1.21  2004/04/05 22:07:08  korpela
//
// Segfault problem fixed?
//
// Revision 1.17  2003/12/01 23:42:05  korpela
// Under some compilers template parameters of type char [] weren't getting
// cast to char *.  Template functions now use &(array[0]) to ensure correct
// type is used.
//
//

#ifndef _XML_UTIL_H_
#define _XML_UTIL_H_

#include "sah_config.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include "error_numbers.h"

// Just in case, undef min and max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif



typedef enum tag_xml_encoding {
  _x_xml_entity=0,
  _x_xml_cdata,
  _x_xml_values,
  _quoted_printable,
  _base64,
  _x_base85,
  _x_setiathome,
  _x_hex,
  _x_csv,
  _x_uuencode,
  _8bit,
  _binary
} xml_encoding;

const char * const xml_encoding_names[]= {
      "x-xml-entity",
      "x-xml-cdata",
      "x-xml-values",
      "quoted-printable",
      "base64",
      "x-base85",
      "x-setiathome",
      "x-hex",
      "x-csv",
      "x-uuencode",
      "8bit",
      "binary"
    };

#if 0

// the xml_ostream class is an ostream, which can be constructed
// from an existing ostream (i.e. cout).  When constructed,
// an xml header and the opening tag are written.  When destructed,
// the closing tag is written.
class xml_ostream {
  public:
    xml_ostream(ostream &o, const char *tag);
    ~xml_ostream();
    template <typename T>
    xml_ostream &operator <<(const T &t) { os << t; return *this; };
  private:
    void write_head();
    void write_foot();
    std::string my_tag;
    std::ostream &os;
};

// the xml_ofstream class is an ofstream.  When the file is opened,
// an xml header and the opening tag are written.  Upon close,
// the closing tag is written.
class xml_ofstream {
  public:
    xml_ofstream();
    explicit xml_ofstream(const char *filename, const char *tag,
                          ios_base::openmode m=ios_base::out|ios_base::binary);
    ~xml_ofstream();
    void open(const char *p, const char *tag,
              ios_base::openmode m=ios_base::out|ios_base::binary);
    void close();
  private:
    void write_head();
    void write_foot();
    std::string my_tag;
    std::ofstream &os;
};

// the xml_istream class is an istream that can be constructed from
// an existing istream.   When constructed, the stream is read until
// the opening tag or end of file is found.  This is really only useful
// for reading XML from stdin.
class xml_istream {
  public:
    explicit xml_istream(istream &i, const char *tag=0);
    ~xml_istream();
    operator istream &() {return is;};
  private:
    void seek_head();
    std::string my_tag;
    std::istream &is;
};
// the xml_ifstream class is an ifstream.  When the file is opened,
// the file pointer is set after the opening tag.  An attempt to
// read past the closing tag will fail as if the end of the file has
// been reached.  If no tag is given, it will assume the first tag
// found is the main tag.

#ifndef HAVE_STD_POS_TYPE
typedef off_t pos_type;
#endif

#ifndef HAVE_STD_OFF_TYPE
typedef off_t off_type;
#endif

class xml_ifstream {
  public:
    xml_ifstream();
    explicit xml_ifstream(const char *filename, const char *tag=0,
                          ios_base::openmode m=ios_base::in|ios_base::binary);
    ~xml_ifstream();
    void open(const char *filename, const char *tag=0,
              ios_base::openmode m=ios_base::in|ios_base::binary);
    xml_ifstream &seekg(pos_type p);
    xml_ifstream &seekg(off_type o, ios_base::seekdir d);
    pos_type tellg();
    bool eof();
  private:
    void seek_head();
    std::string my_tag;
    std::pos_type xml_start;
    std::pos_type xml_end;
    std::ifstream &ifs;
};

#endif // 0

#define XML_ENCODING "iso-8859-1"

static const char * const xml_header=
  "<?xml version=\"1.0\" encoding=\""XML_ENCODING"\"?>\n";

// XML entity for tranlation table (not wchar_t compatible)
struct xml_entity {
  unsigned char c;
  const char *s;
};

// change the xml indent level (number of spaces) by adding or subtracting
// "i" spaces.  return a string of spaces corresponding to the current xml
// indent level.
std::string xml_indent(int i=0);
static const int XML_MAX_INDENT=40;
extern int xml_indent_level;


// decode an XML character string.  Return a the decoded string in a vector
// (null not necessarily a terminator).
//template <typename T>
//vector<T> xml_decode_string(const char *input, size_t length=0,
//    const char *encoding="x_xml_entity");

// do the same thing, but get the length and encoding type from the
// xml tag properties.
template <typename T>
std::vector<T> xml_decode_field(const std::string &input, const char *tag);

// encode an XML character string.  Return the encoded string.
//template <typename T>
//string xml_encode_string(const T *input, size_t n_elements=0,
//    xml_encoding encoding=_x_xml_entity);

template <typename T>
std::string xml_encode_string(const std::vector<T> &input,
                                     xml_encoding encoding=_x_xml_entity) {
  return xml_encode_string(&(*(input.begin())),input.size(),encoding);
}

#include <cctype>
#include <vector>
#include <string>
#include <sstream>

extern const char *encode_arr;
extern const char *encode_arr85;
bool isencchar(char c);
bool isencchar85(char c);

template <typename T>
std::string base64_encode(const T *tbin, size_t n_elements) {
  size_t nbytes=n_elements*sizeof(T);
  const unsigned char *bin=(const unsigned char *)(tbin);
  int count=0, offset=0, nleft;
  const char crlf[]= {0xa,0xd,0x0};
  std::string rv("");
  rv.reserve(nbytes*4/3+nbytes*2/57);
  char c[5];
  for (nleft = (int)nbytes; nleft > 0; nleft -= 3) {
    int i;
    c[0] = (bin[offset]>>2) & 0x3f ;     // 6
    c[1] = (bin[offset]<<4) & 0x3f | ((bin[offset+1]>>4)&0xf); // 2+4
    c[2] = ((bin[offset+1]<<2)&0x3f) | ((bin[offset+2]>>6)&0x3);// 4+2
    c[3] = bin[offset+2]&0x3f;    // 6
    for (i=0;i<((nleft>3)?4:(nleft+1));i++) c[i]=encode_arr[c[i]];
    for (;i<4;i++) c[i]='=';
    c[4]=0;
    rv+=c;
    offset += 3;
    count += 4;
    if (count == 76 ) {
      count = 0;
      rv+=crlf;
    }
  }
  rv+=crlf;
  return rv;
}

template <typename T>
std::vector<T> base64_decode(const char *data, size_t nbytes) {
  const char *p=data,*eol,*eol2,*eos;
  const char cr=0xa,lf=0xd;
  char in[4],c[3];
  int i;
  std::vector<unsigned char> rv;
  rv.reserve(nbytes*3/4);
  while (p<(data+nbytes)) {
    while (*p && (p<(data+nbytes)) && !isencchar(*p)) {
      *p++;
    }
    if (!(*p) || (p>=(data+nbytes))) break;
    eol=strchr(p,cr);
    eol2=strchr(p,lf);
    eos=p+strlen(p);
    if (eol) {
      eol=std::min(eol,eos);
    } else {
      eol=eos;
    }
    if (eol && eol2) {
      eol=std::min(eol,eol2);
    }
    for (;p<(eol-1);p+=4) {
      for ( i=0;i<4;i++) {
        if ((p[i]>='A') && (p[i]<='Z')) {
          in[i]=p[i]-'A';
        } else if ((p[i]>='a') && (p[i]<='z')) {
          in[i]=p[i]-'a'+26;
        } else if ((p[i]>='0') && (p[i]<='9')) {
          in[i]=p[i]-'0'+52;
        } else {
          switch (p[i]) {
            case '+': in[i]=62;
              break;
            case '/': in[i]=63;
              break;
            default : in[i]=0;
          }
        }
      }
      c[0]=(in[0]<<2) | ((in[1] >> 4) & 0x3);
      c[1]=(in[1]<<4) | ((in[2] >> 2) & 0xf);
      c[2]=(in[2]<<6) | in[3];
      for ( i=0;i<3;i++) rv.push_back(c[i]);
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}

template <typename T>
std::string base85_encode(const T *tbin, size_t n_elements) {
  size_t nbytes=n_elements*sizeof(T);
  const unsigned char *bin=(const unsigned char *)(tbin);
  int count=0;
  const char crlf[]= {0xa,0xd,0x0};
  std::string rv("");
  rv.reserve(nbytes*4/3+nbytes*2/57);
  char c[6];
  int n_pads;
  unsigned int j=0;
  while (j<nbytes) {
    unsigned int i;
    unsigned long val=0;
    for (i=0;i<(((nbytes-j)>3)?4:(nbytes-j));i++) val=(val<<8)+bin[j+i];
    if (val) {
      for (n_pads=4-i;i<4;i++) val*=((i==3)?84:85);
    }
    if (val == 0) {
      c[0]='z';     // If the word is entirely zero use a single digit
      c[1]=0;       // zero pad of 'z'
    } else {
      for (i=4;i<5;i--) {
        c[i]=(char)(val % ((i==4)?84:85));  // First division is by 84 to prevent
        val/= ((i==4)?84:85);               // having a pad in the final digit.
      }
      if (c[0]==83) {               // need to change a high order 'z' into
        c[0]=84;                        // an "_" so it won't look like a zero word.
      }
      for (i=0;i<5;i++) c[i]=encode_arr85[c[i]];
      for (i=5-n_pads;i<5;i++) c[i]='_'; // add pad characters
      c[5]=0;
    }
    j+=4;
    if (count>74) {
      rv+=crlf;
      count=0;
    }
    count+=(int)strlen(c);
    rv+=c;
  }
  return rv;
}

template <typename T>
std::vector<T> base85_decode(const char *data, size_t nbytes) {
  const char *p=data,*eol,*eol2,*eos;
  const char cr=0xa,lf=0xd;
  unsigned long val;
  int npads,i;
  std::vector<unsigned char> rv;
  rv.reserve(nbytes*4/5);
  while (p<(data+nbytes)) {
    while (*p && (p<(data+nbytes)) && !isencchar85(*p)) {
      *p++;
    }
    if (!(*p) || (p>=(data+nbytes))) break;
    eol=strchr(p,cr);
    eol2=strchr(p,lf);
    eos=p+strlen(p);

    if (eol) {
      eol=std::min(eol,eos);
    } else {
      eol=eos;
    }
    if (eol && eol2) {
      eol=std::min(eol,eol2);
    }

    while (p<eol) {
      val=0;
      npads=0;
      switch (*p) {
        case 'z': for (i=0;i<4;i++) rv.push_back((unsigned char)0);
          p++;
          break;
        default:
          i=5;
          while (i-->0) {
            if (p[i]!='_') break;
            npads++;
          }
          for (i=0;i<std::min((long)(eol-p),(long)(5-npads));i++) {
            val*=((i==4)?84:85);
            if ((p[i]>='0') && (p[i]<='9')) {
              val+=p[i]-'0';
            } else if ((p[i]>='A') && (p[i]<='Z')) {
              val+=p[i]-'A'+10;
            } else if ((p[i]>='a') && (p[i]<='y')) {
              val+=p[i]-'a'+36;
            } else {
              for (int j=61; j<85; j++) {
                if (p[i]==encode_arr85[j]) {
                  val+=j;
                  j=85;
                }
              }
              if ((i==0) && (p[i]=='_')) val--;
            }
          }
      }
      val<<=(npads*8);
      char c[5];
      c[4]=0;
      for (int i=3;i>=0;i--) {
        c[i]=(char)(val & 0xff);
        val>>=8;
      }
      for (int i=0;i<4;i++) {
        rv.push_back(c[i]);
      }
      p+=5;
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}


template <typename T>
std::string x_setiathome_encode(const T *tbin, size_t n_elements) {
  size_t nbytes=n_elements*sizeof(T);
  const unsigned char *bin=(const unsigned char *)(tbin);
  int count=0, offset=0, nleft;
  const char cr=0xa;
  std::string rv("");
  rv.reserve(nbytes*4/3+nbytes*2/48);
  rv+="\n";
  char c[5];
  for (nleft = (int)nbytes; nleft > 0; nleft -= 3) {
    c[0] = bin[offset]&0x3f;     // 6
    c[1] = (bin[offset]>>6) | (bin[offset+1]<<2)&0x3f; // 2+4
    c[2] = ((bin[offset+1]>>4)&0xf) | (bin[offset+2]<<4)&0x3f;// 4+2
    c[3] = bin[offset+2]>>2;    // 6
    for (int i=0;i<4;i++) c[i]+=0x20;
    c[4]=0;
    rv+=c;
    offset += 3;
    count += 4;
    if (count == 64) {
      count = 0;
      rv+=cr;
    }
  }
  rv+=cr;
  return rv;
}


template <typename T>
std::vector<T> x_setiathome_decode(const char *data, size_t nbytes) {
  const char *p=data,*eol,*eol2,*eos;
  char in[4],c[3];
  int i;
  std::vector<unsigned char> rv;
  rv.reserve(nbytes*3/4);
  while (p<(data+nbytes)) {
    while ((*p<0x20) || (*p>0x60)) {
      *p++;
    }
    eol=strchr(p,'\n');
    eol2=strchr(p,'\r');
    eos=p+strlen(p);
    if (eol) {
      eol=std::min(eol,eos);
    } else {
      eol=eos;
    }
    if (eol && eol2) {
      eol=std::min(eol,eol2);
    }
    for (;p<(eol-1);p+=4) {
      memcpy(in,p,4);
      for ( i=0;i<4;i++) in[i]-=0x20;
      c[0]=in[0]&0x3f | in[1]<<6;
      c[1]=in[1]>>2 | in[2]<<4;
      c[2]=in[2]>>4 | in[3]<<2;
      for ( i=0;i<3;i++) rv.push_back(c[i]);
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}

template <typename T>
std::string quoted_printable_encode(const T *tbin, size_t n_elements) {
  size_t nbytes=n_elements*sizeof(T);
  const unsigned char *bin=(const unsigned char *)(tbin);
  int line_len=0;
  const char crlf[]= {'=',0xa,0xd,0x0};
  std::string rv("");
  rv.reserve(nbytes*4/3+nbytes*2/48);
  for (size_t i=0;i<nbytes;i++) {
    if (isprint(bin[i]) && (bin[i]!='=')) {
      if (++line_len > 74) {
        rv+=crlf;
        line_len=1;
      }
      rv+=bin[i];
    } else {
      line_len+=3;
      if (line_len>72) {
        rv+=crlf;
        line_len=3;
      }
      char buf[4];
      sprintf(buf,"=%.2X",bin[i]);
      rv+=buf;
    }
  }
  return rv;
}

template <typename T>
std::vector<T> quoted_printable_decode(const char* data, size_t nbytes) {
  std::vector<unsigned char> rv;
  rv.reserve(strlen(data));
  size_t i=0;
  while (i<nbytes) {
    if (data[i]!='=') {
      rv.push_back(data[i]);
      i++;
    } else {
      if (!((data[i+1] == 0xa) && (data[i+2]==0xd))) {
        unsigned int c;
        sscanf(data+i+1,"%2X",&c);
        rv.push_back(c);
      }
      i+=3;
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}

template <typename T>
std::string x_hex_encode(const T *tbin, size_t n_elements) {
  size_t nbytes=n_elements*sizeof(T);
  const unsigned char *bin=(const unsigned char *)(tbin);
  std::string rv;
  int count=0;
  rv.reserve(nbytes*2+nbytes*2/76);
  for (unsigned int i=0; i<nbytes; i++) {
    char buf[3];
    sprintf(buf,"%.2x",bin[i]);
    rv+=buf;
    count+=2;
    if (count == 76) {
      count=0;
      rv+='\n';
    }
  }
  return rv;
}

template <typename T>
std::vector<T> x_hex_decode(const char *data, size_t nbytes) {
  std::vector<unsigned char> rv;
  rv.reserve(nbytes/2);
  unsigned int i=0;
  while (i<nbytes) {
    unsigned int c;
    while (!isxdigit(data[i])) i++;
    sscanf(data+i,"%2x",&c);
    i+=2;
    rv.push_back(static_cast<unsigned char>(c));
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}


std::string x_csv_encode_char(const unsigned char *bin, size_t nelements);

template <typename T>
std::string x_csv_encode(const T *bin, size_t nelements) {
  std::ostringstream rv("");
  long lastlen,i;

  if (sizeof(T)==1) // if T is a char, print in another way
    return x_csv_encode_char((const unsigned char *)bin,nelements);
  
  rv << std::endl << xml_indent(2);  // TMR moved here to fix PoT format
  lastlen=(long)rv.str().size();       // TMR
  for (i=0;i<(static_cast<long>(nelements)-1);i++) {
    rv << bin[i] << ',';
    if ((static_cast<int>(rv.str().size())
      -(lastlen-std::min(xml_indent_level,XML_MAX_INDENT)))>73) {  // TMR
      rv << std::endl << xml_indent();
      lastlen=(long)rv.str().size();
    }
  }
  rv << bin[i] << "\n" << xml_indent(-2);

  return rv.str();
}


template <typename T>
std::vector<T> x_csv_decode(const char *data, size_t nbytes) {
  std::vector<T> rv;
  while (!isdigit(*data)) {
    data++;
    nbytes--;
  }
  std::istringstream in(std::string(data,nbytes));
  bool ischar=(sizeof(T)==1);

  while (in) {
    T t;
    if (!ischar) {
      in >> t;
    } else {
      int i;
      in >> i;
      t=i & 0xff;
    }
    rv.push_back(t);
    char c=' ';
    while (in && !isdigit(c)) {
      in.get(c);
    }
    if (isdigit(c)) in.putback(c);
  }
  return rv;
}


std::string encode_char(unsigned char c);
unsigned char decode_char(const char *s);

template <typename T>
std::vector<T> x_xml_entity_decode(const char *input, size_t length) {
  size_t i;
  char c;
  if (!length) {
    // We're going to decode until we see a null.  Including the null.
    length=strlen((const char *)input);
  }
  std::vector<unsigned char> rv;
  char *p;
  rv.reserve(length);
  for (i=0; i<length; i++) {
    if (input[i]=='&') {
      rv.push_back(c=decode_char(input+i));
      if ((c!='&') || !strncmp((const char *)(input+i),"&amp;",5)) {
        p=strchr((char *)(input+i),';');
        i=(p-input);
      }
    } else {
      rv.push_back(input[i]);
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}

template <typename T>
std::string x_xml_entity_encode(const T *tbin, size_t n_elements) {
  size_t length=n_elements*sizeof(T);
  const unsigned char *input=(const unsigned char *)(tbin);
  unsigned int i;
  std::string rv;
  rv.reserve(length);
  for (i=0; i<length; i++) {
    if (isprint(input[i])) {
      switch (input[i]) {
        case '>':
        case '<':
        case '&':
        case '\'':
        case '"':
          rv+=encode_char(input[i]);
          break;
        default:
          rv+=input[i];
      }
    } else {
      char buf[16];
      sprintf(buf,"&#%.3d;",input[i]);
      rv+=buf;
    }
  }
  return rv;
}

template <typename T>
std::string x_xml_values_encode(const T *bin, size_t n_elements) {
  std::ostringstream rv("");
  unsigned int i;
  for (i=0;i<n_elements-1;i++) {
    rv << bin[i] ;
    if (sizeof(T) < 9) rv << ',';
  }
  rv << bin[i] ;
  return rv.str();
}

template <typename T>
std::vector<T> x_xml_values_decode(const char *data, size_t length) {
  std::istringstream r(std::string(data,length));
  std::vector<T> rv;
  T t;
  while (!r.eof()) {
    r >> t ;
    rv.push_back(t);
    while ((isspace(r.peek()) || (r.peek() == ',')) && !r.eof()) {
      char c;
      r.get(c);
    }
  }
  return rv;
}

template <typename T>
std::string x_xml_cdata_encode(const T *tbin, size_t n_elements) {
  size_t length=n_elements*sizeof(T);
  const unsigned char *input=(const unsigned char *)(tbin);
  unsigned int i;
  std::string rv("<![CDATA[");
  rv.reserve(length);
  for (i=0; i<length; i++) {
    if (input[i]>0x1f) {
      switch (input[i]) {
        case ']':
          if (((length-i)>1) && (input[i+1]==']') && (input[i+2]=='>')) {
            rv+="&&endcdt;";
          } else {
            rv+=']';
          }
          break;
        default:
          rv+=input[i];
      }
    } else {
      char buf[16];
      sprintf(buf,"&&#%.2d;",input[i]);
      rv+=buf;
    }
  }
  rv+="]]>";
  return rv;
}

template <typename T>
std::vector<T> x_xml_cdata_decode(const char *input, size_t length) {
  size_t i;
  char c;
  if (!length) {
    // We're going to decode until we see a null.  Including the null.
    length=strlen(input);
  }
  std::vector<unsigned char> rv;
  char *p;
  rv.reserve(length);
  for (i=0; i<length; i++) {
    if (input[i]=='&') {
      if (((length-i)>8) && !strncmp((const char *)(input+i),"&&endcdt;",9)) {
        rv.push_back(']');
        rv.push_back(']');
        rv.push_back('>');
        i+=8;
      } else {
        if (input[i+1]=='&') {
          rv.push_back(c=decode_char(input+i+1));
          if ((c!='&') || !strncmp((const char *)(input+i+1),"&amp;",5)) {
            p=strchr((char *)(input+i+1),';');
            i=(p-input);
          }
        } else {
          rv.push_back(input[i]);
        }
      }
    } else {
      rv.push_back(input[i]);
    }
  }
  return std::vector<T>((T *)(&(rv[0])),(T *)(&(rv[0]))+rv.size()/sizeof(T));
}

template <typename T>
std::vector<T> x_uudecode(const char *data, size_t nbytes) {
  std::vector<T> rv;
  return rv;
}

template <typename T>
std::string x_uuencode(const T *data, size_t nbytes) {
  std::string rv;
  return rv;
}

inline int xml_encoding_from_string(const char *enc_string) {
  int i=_x_xml_entity;
  const char *p="xqb8";
  // find the start to the encoding string (maybe prepended by
  // quote or whitespace.
  while (*enc_string && (strchr(p,*enc_string) == NULL)) enc_string++;
  do {
    if (!strncmp(enc_string,xml_encoding_names[i],strlen(xml_encoding_names[i])))
      break;
  } while (i++ != _binary);
  return i;
}

template <typename T>
std::vector<T> xml_decode_string(const char *input,
                                 size_t length=0, const char *encoding="x_xml_entity") {
  int i=xml_encoding_from_string(encoding);
  switch (i) {
    case _x_xml_entity:
      return x_xml_entity_decode<T>(input,length);
    case _x_xml_cdata:
      return x_xml_cdata_decode<T>(input,length);
    case _x_xml_values:
      return x_xml_values_decode<T>(input,length);
    case _quoted_printable:
      return quoted_printable_decode<T>(input,length);
    case _base64:
      return base64_decode<T>(input,length);
    case _x_base85:
      return base85_decode<T>(input,length);
    case _x_setiathome:
      return x_setiathome_decode<T>(input,length);
    case _x_hex:
      return x_hex_decode<T>(input,length);
    case _x_csv:
      return x_csv_decode<T>(input,length);
    case _x_uuencode:
      return x_uudecode<T>(input,length);
    case _8bit:
    case _binary:
      return std::vector<T>((const T *)input,(const T *)input+length/sizeof(T));
    default:
      return x_xml_entity_decode<T>(input,length);
  }
}

template <typename T>
std::vector<T> xml_decode_field(const std::string &input, const char *tag) {
  std::string start_tag("<"),end_tag("</");
  start_tag+=tag;
  start_tag+=' ';
  end_tag+=tag;
  std::string::size_type start,endt,enc,len;
  if (((start=input.find(start_tag))==std::string::npos) ||
      ((endt=input.find(end_tag,start))==std::string::npos) ||
      ((enc=input.find("encoding=\"",start))==std::string::npos)) {
    throw ERR_XML_PARSE;
  }
  unsigned int length=0;
  if ((len=input.find("length=",start)!=std::string::npos))
    length=atoi(&(input.c_str()[len+strlen("length=")]));
  const char *encoding=input.c_str()+enc+strlen("encoding=\"");
  start=input.find('>',start)+1;
  if (!length) {
    length=(unsigned int)endt - (unsigned int)start;
  }
  return (xml_decode_string<T>(&(input.c_str()[start]),length,encoding));
}


template <typename T>
std::string xml_encode_string(const T *input,
                              size_t length=0, xml_encoding encoding=_x_xml_entity) {
  switch (encoding) {
    case _x_xml_entity:
      return x_xml_entity_encode<T>(input,length);
    case _x_xml_cdata:
      return x_xml_cdata_encode<T>(input,length);
    case _x_xml_values:
      return x_xml_values_encode<T>(input,length);
    case _quoted_printable:
      return quoted_printable_encode<T>(input,length);
    case _base64:
      return base64_encode<T>(input,length);
    case _x_base85:
      return base85_encode<T>(input,length);
    case _x_setiathome:
      return x_setiathome_encode<T>(input,length);
    case _x_hex:
      return x_hex_encode<T>(input,length);
    case _x_csv:
      return x_csv_encode<T>(input,length);
    case _x_uuencode:
      return x_uuencode<T>(input,length);
    case _8bit:
    case _binary:
      return std::string((const char *)(input),length*sizeof(T));
    default:
      return x_xml_entity_encode<T>(input,length);
  }
}

extern bool xml_match_tag(const char*, const char*);
extern bool xml_match_tag(const std::string &, const char*);
extern bool extract_xml_record(const std::string &field, const char *tag, std::string &record);

#endif
//
// $Log: xml_util.h,v $
// Revision 1.8.2.6  2007/05/31 22:03:31  korpela
// *** empty log message ***
//
// Revision 1.8.2.5  2007/03/09 00:21:15  vonkorff
// Fixed missing trailing null in base64 encoding.
//
// Revision 1.8.2.4  2006/12/14 22:26:30  korpela
// *** empty log message ***
//
// Revision 1.8.2.3  2005/12/01 00:01:01  korpela
// Changed to an Intel compile of fftw3.dll.  Hopefully this will fix the problem
// with crashes on Pentium-MMX and earlier.
//
// Added a Dev-C++ project and modified source files to allow seti_boinc to compile
// under MinGW.
//
// Revision 1.8.2.2  2005/10/17 22:33:33  korpela
// Continuing previous fix
//
// Revision 1.8.2.1  2005/10/17 22:14:49  korpela
// Fixed bugs with XML formatting and CSV vector input.  The XML formatting bug
// fixes were submitted by Tetsuji "Maverick" Rai.  The CSV input problem was due
// to my misunderstanding of the istream::operator void *().  I had thought the
// operator returned NULL if the past operation had failed.  Instead it returns
// NULL if the NEXT operation will fail.
//
// Revision 1.8  2004/12/07 23:42:23  korpela
// Added undef of min and max in case they are defined in a previously included
// header file.
//
// Revision 1.7  2004/11/18 19:17:04  korpela
// Fixed base64 and base85 decoding.
//
// Revision 1.6  2004/10/29 04:35:21  korpela
// *** empty log message ***
//
// Revision 1.5  2004/10/21 22:02:48  korpela
// *** empty log message ***
//
// Revision 1.4  2004/06/30 20:52:29  korpela
// *** empty log message ***
//
// Revision 1.3  2004/06/24 08:55:29  quarl
// *** empty log message ***
//
// Revision 1.2  2004/06/12 18:38:30  rwalton
// *** empty log message ***
//
// Revision 1.1  2004/06/11 17:43:22  davea
// *** empty log message ***
//
// Revision 1.21  2004/04/05 22:07:08  korpela
//
// Segfault problem fixed?
//
// Revision 1.20  2004/03/06 09:45:25  rwalton
// *** empty log message ***
//
// Revision 1.19  2004/02/05 05:32:22  quarl
// *** empty log message ***
//
// Revision 1.18  2004/01/22 17:57:41  davea
// *** empty log message ***
//
// Revision 1.17  2003/12/01 23:42:05  korpela
// Under some compilers template parameters of type char [] weren't getting
// cast to char *.  Template functions now use &(array[0]) to ensure correct
// type is used.
//
// Revision 1.16  2003/10/29 20:08:50  korpela
// *** empty log message ***
//
// Revision 1.15  2003/10/27 23:07:34  korpela
// *** empty log message ***
//
// Revision 1.14  2003/10/27 20:07:12  korpela
// *** empty log message ***
//
// Revision 1.13  2003/10/25 18:20:03  korpela
// *** empty log message ***
//
// Revision 1.12  2003/10/24 16:58:11  korpela
// *** empty log message ***
//
// Revision 1.11  2003/10/23 15:39:54  korpela
// no message
//
// Revision 1.10  2003/10/22 23:11:49  davea
// *** empty log message ***
//
// Revision 1.9  2003/10/22 22:36:52  jeffc
// jeffc - init xml_encode/decode_string in the definition, not the prototype
//
// Revision 1.8  2003/10/22 18:13:39  korpela
// *** empty log message ***
//
// Revision 1.7  2003/10/22 17:43:10  korpela
// *** empty log message ***
//
// Revision 1.6  2003/10/22 15:24:10  korpela
// *** empty log message ***
//
// Revision 1.5  2003/10/22 03:09:55  korpela
// *** empty log message ***
//
// Revision 1.4  2003/10/21 18:14:36  korpela
// *** empty log message ***
//
//
