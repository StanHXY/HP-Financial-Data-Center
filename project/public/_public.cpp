/*****************************************************************************************/
/*   Program name: _public.cpp, this program is the definition file for common functions and classes in the development framework. */
/*****************************************************************************************/

#include "_public.h"  

// Safe strcpy function.
// dest: Target string, no need to initialize, as there is initialization code in STRCPY function.
// destlen: Memory size occupied by the target string dest.
// src: Source string.
// Return value: Address of the target string dest.
// Note that content exceeding the capacity of dest will be discarded.
char *STRCPY(char* dest, const size_t destlen, const char* src)
{
  if (dest == 0) return 0; // Check for NULL pointer.
  memset(dest, 0, destlen); // Initialize dest.
  // memset(dest, 0, sizeof(dest)); // This cannot be written like this, as sizeof(dest) will always be 8 on a 64-bit system.
  if (src == 0) return dest;

  if (strlen(src) > destlen - 1) strncpy(dest, src, destlen - 1);
  else strcpy(dest, src);

  return dest;
}

// Safe strncpy function.
// dest: Target string, no need to initialize, as there is initialization code in STRCPY function.
// destlen: Memory size occupied by the target string dest.
// src: Source string.
// n: Number of bytes to be copied.
// Return value: Address of the target string dest.
// Note that content exceeding the capacity of dest will be discarded.
char *STRNCPY(char* dest, const size_t destlen, const char* src, size_t n)
{
  if (dest == 0) return 0; // Check for NULL pointer.
  memset(dest, 0, destlen); // Initialize dest.
  // memset(dest, 0, sizeof(dest)); // This cannot be written like this, as sizeof(dest) will always be 8 on a 64-bit system.
  if (src == 0) return dest;

  if (n > destlen - 1) strncpy(dest, src, destlen - 1);
  else strncpy(dest, src, n);

  return dest;
}

// Safe strcat function.
// dest: Target string, note that if dest has never been used, it needs to be initialized at least once.
// destlen: Memory size occupied by the target string dest.
// src: String to be appended.
// Return value: Address of the target string dest.
// Note that content exceeding the capacity of dest will be discarded.
char *STRCAT(char* dest, const size_t destlen, const char* src)
{
  if (dest == 0) return 0; // Check for NULL pointer.
  if (src == 0) return dest;

  unsigned int left = destlen - 1 - strlen(dest);

  if (strlen(src) > left) { strncat(dest, src, left); dest[destlen - 1] = 0; }
  else strcat(dest, src);

  return dest;
}


// Safe strncat function.
// dest: Target string, note that if dest has never been used, it needs to be initialized at least once.
// destlen: Memory size occupied by the target string dest.
// src: String to be appended.
// n: Number of bytes to be appended.
// Return value: Address of the target string dest.
// Note that content exceeding the capacity of dest will be discarded.
char *STRNCAT(char* dest, const size_t destlen, const char* src, size_t n)
{
  if (dest == 0) return 0; // Check for NULL pointer.
  if (src == 0) return dest;

  size_t left = destlen - 1 - strlen(dest);

  if (n > left) { strncat(dest, src, left); dest[destlen - 1] = 0; }
  else strncat(dest, src, n);

  return dest;
}

// Safe sprintf function.
// Format the variable arguments (...) according to the format described in fmt and output to the dest string.
// dest: Output string, no need to initialize, it will be initialized in SPRINTF function.
// destlen: Memory size occupied by the output string dest. If the length of the formatted string content is greater than destlen-1, the content after destlen-1 will be discarded.
// fmt: Format control description.
// ...: Parameters filled into the format control description fmt.
// Return value: Length of the formatted content, usually not important for programmers.
int SPRINTF(char *dest, const size_t destlen, const char *fmt, ...)
{
  if (dest == 0) return -1; // Check for NULL pointer.

  memset(dest, 0, destlen);
  // memset(dest, 0, sizeof(dest)); // This cannot be written like this, as sizeof(dest) will always be 8 on a 64-bit system.

  va_list arg;
  va_start(arg, fmt);
  int ret = vsnprintf(dest, destlen, fmt, arg);
  va_end(arg);

  return ret;
}

// Safe snprintf function.
// Format the variable arguments (...) according to the format described in fmt and output to the dest string.
// dest: Output string, no need to initialize, it will be initialized in SNPRINTF function.
// destlen: Memory size occupied by the output string dest. If the length of the formatted string content is greater than destlen-1, the content after destlen-1 will be discarded.
// n: The formatted string will be truncated to n-1 and stored in dest. If n > destlen, it will be truncated to destlen-1.
// fmt: Format control description.
// ...: Parameters filled into the format control description fmt.
// Return value: Length of the formatted content, usually not important for programmers.
int SNPRINTF(char *dest, const size_t destlen, size_t n, const char *fmt, ...)
{
  if (dest == 0) return -1; // Check for NULL pointer.

  memset(dest, 0, destlen);
  // memset(dest, 0, sizeof(dest)); // This cannot be written like this, as sizeof(dest) will always be 8 on a 64-bit system.

  int len = n;
  if (n > destlen) len = destlen;

  va_list arg;
  va_start(arg, fmt);
  int ret = vsnprintf(dest, len, fmt, arg);
  va_end(arg);

  return ret;
}


// Delete the specified character from the left of the string.
// str: String to be processed.
// chr: Character to be deleted.
void DeleteLChar(char *str, const char chr)
{
  if (str == 0) return;
  if (strlen(str) == 0) return;

  char strTemp[strlen(str) + 1];

  int iTemp = 0;

  memset(strTemp, 0, sizeof(strTemp));
  strcpy(strTemp, str);

  while (strTemp[iTemp] == chr) iTemp++;

  memset(str, 0, strlen(str) + 1);

  strcpy(str, strTemp + iTemp);

  return;
}

// Delete the specified character from the right of the string.
// str: String to be processed.
// chr: Character to be deleted.
void DeleteRChar(char *str, const char chr)
{
  if (str == 0) return;
  if (strlen(str) == 0) return;

  int istrlen = strlen(str);

  while (istrlen > 0)
  {
    if (str[istrlen - 1] != chr) break;

    str[istrlen - 1] = 0;

    istrlen--;
  }
}

// Delete the specified character from both sides of the string.
// str: String to be processed.
// chr: Character to be deleted.
void DeleteLRChar(char *str, const char chr)
{
  DeleteLChar(str, chr);
  DeleteRChar(str, chr);
}

// Convert lowercase letters in the string to uppercase, ignoring non-alphabetic characters.
// str: String to be converted, supports both char[] and string types.
void ToUpper(char *str)
{
  if (str == 0) return;

  if (strlen(str) == 0) return;

  int istrlen = strlen(str);

  for (int ii = 0; ii < istrlen; ii++)
  {
    if ((str[ii] >= 'a') && (str[ii] <= 'z')) str[ii] = str[ii] - 32;
  }
}

void ToUpper(string &str)
{
  if (str.empty()) return;

  char strtemp[str.size() + 1];

  STRCPY(strtemp, sizeof(strtemp), str.c_str());

  ToUpper(strtemp);

  str = strtemp;

  return;
}


// Convert uppercase letters in the string to lowercase, ignoring non-alphabetic characters.
// str: String to be converted, supports both char[] and string types.
void ToLower(char *str)
{
  if (str == 0) return;

  if (strlen(str) == 0) return;

  int istrlen = strlen(str);

  for (int ii = 0; ii < istrlen; ii++)
  {
    if ((str[ii] >= 'A') && (str[ii] <= 'Z')) str[ii] = str[ii] + 32;
  }
}

void ToLower(string &str)
{
  if (str.empty()) return;

  char strtemp[str.size() + 1];

  STRCPY(strtemp, sizeof(strtemp), str.c_str());

  ToLower(strtemp);

  str = strtemp;

  return;
}

// String replacement function.
// Replace str1 with str2 in the string str.
// str: String to be processed.
// str1: Old content.
// str2: New content.
// bloop: Whether to execute the replacement repeatedly.
// Note:
// 1. If str2 is longer than str1, the length of str will be increased after replacement, so make sure str has enough capacity to avoid memory overflow.
// 2. If str2 contains the content of str1 and bloop is true, there will be a logical error, and no replacement will be executed.
void UpdateStr(char *str, const char *str1, const char *str2, bool bloop)
{
  if (str == 0) return;
  if (strlen(str) == 0) return;
  if ((str1 == 0) || (str2 == 0)) return;

  // If bloop is true and str2 contains the content of str1, return directly, as it will enter an infinite loop and eventually cause memory overflow.
  if ((bloop == true) && (strstr(str2, str1) > 0)) return;

  // Allocate as much space as possible, but there is still a possibility of memory overflow, it is recommended to optimize it into string.
  int ilen = strlen(str) * 10;
  if (ilen < 1000) ilen = 1000;

  char strTemp[ilen];

  char *strStart = str;

  char *strPos = 0;

  while (true)
  {
    if (bloop == true)
    {
      strPos = strstr(str, str1);
    }
    else
    {
      strPos = strstr(strStart, str1);
    }

    if (strPos == 0) break;

    memset(strTemp, 0, sizeof(strTemp));
    STRNCPY(strTemp, sizeof(strTemp), str, strPos - str);
    STRCAT(strTemp, sizeof(strTemp), str2);
    STRCAT(strTemp, sizeof(strTemp), strPos + strlen(str1));
    strcpy(str, strTemp);

    strStart = strPos + strlen(str2);
  }
}

// Extract numbers from a string and store them in another string.
// src: Source string.
// dest: Destination string.
// bsigned: Whether to include signs (+ and -), true-include; false-exclude.
// bdot: Whether to include the decimal point, true-include; false-exclude.
void PickNumber(const char *src, char *dest, const bool bsigned, const bool bdot)
{
  if (dest == 0) return; // Check for null pointer.
  if (src == 0)
  {
    strcpy(dest, "");
    return;
  }

  char strtemp[strlen(src) + 1];
  memset(strtemp, 0, sizeof(strtemp));
  strcpy(strtemp, src);
  DeleteLRChar(strtemp, ' ');

  int ipossrc, iposdst, ilen;
  ipossrc = iposdst = ilen = 0;

  ilen = strlen(strtemp);

  for (ipossrc = 0; ipossrc < ilen; ipossrc++)
  {
    if ((bsigned == true) && (strtemp[ipossrc] == '+'))
    {
      dest[iposdst++] = strtemp[ipossrc];
      continue;
    }

    if ((bsigned == true) && (strtemp[ipossrc] == '-'))
    {
      dest[iposdst++] = strtemp[ipossrc];
      continue;
    }

    if ((bdot == true) && (strtemp[ipossrc] == '.'))
    {
      dest[iposdst++] = strtemp[ipossrc];
      continue;
    }

    if (isdigit(strtemp[ipossrc])) dest[iposdst++] = strtemp[ipossrc];
  }

  dest[iposdst] = 0;
}


// Regular expression to check if a string matches another string.
// str: String to be checked, precise representation of the string, such as file name "_public.cpp".
// rules: Matching rule expression, using asterisk "*" to represent any string, multiple strings are separated by commas, such as "*.h,*.cpp".
// Note that the str parameter does not support "*", while the rules parameter does support "*", and the function will ignore the case of letters when determining whether str matches rules.
bool MatchStr(const string &str, const string &rules)
{
  // If the comparison string is empty, return false
  if (rules.empty()) return false;

  // If the string to be compared is "*", return true
  if (rules == "*") return true;

  int ii, jj;
  int iPOS1, iPOS2;
  CCmdStr CmdStr, CmdSubStr;

  string strFileName, strMatchStr;

  strFileName = str;
  strMatchStr = rules;

  // Convert both strings to uppercase for comparison
  ToUpper(strFileName);
  ToUpper(strMatchStr);

  CmdStr.SplitToCmd(strMatchStr, ",");

  for (ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    // Skip if the string is empty, otherwise it will be matched
    if (CmdStr.m_vCmdStr[ii].empty()) continue;

    iPOS1 = iPOS2 = 0;
    CmdSubStr.SplitToCmd(CmdStr.m_vCmdStr[ii], "*");

    for (jj = 0; jj < CmdSubStr.CmdCount(); jj++)
    {
      // If it is the beginning of the file name
      if (jj == 0)
      {
        if (strncmp(strFileName.c_str(), CmdSubStr.m_vCmdStr[jj].c_str(), CmdSubStr.m_vCmdStr[jj].size()) != 0) break;
      }

      // If it is the end of the file name
      if (jj == CmdSubStr.CmdCount() - 1)
      {
        if (strcmp(strFileName.c_str() + strFileName.size() - CmdSubStr.m_vCmdStr[jj].size(), CmdSubStr.m_vCmdStr[jj].c_str()) != 0) break;
      }

      iPOS2 = strFileName.find(CmdSubStr.m_vCmdStr[jj], iPOS1);

      if (iPOS2 < 0) break;

      iPOS1 = iPOS2 + CmdSubStr.m_vCmdStr[jj].size();
    }

    if (jj == CmdSubStr.CmdCount()) return true;
  }

  return false;
}

CFile::CFile() // Class constructor
{
  m_fp = 0;
  m_bEnBuffer = true;
  memset(m_filename, 0, sizeof(m_filename));
  memset(m_filenametmp, 0, sizeof(m_filenametmp));
}


// Close the file pointer
void CFile::Close()
{
  if (m_fp == 0) return; // Check for null pointer.

  fclose(m_fp); // Close the file pointer

  m_fp = 0;
  memset(m_filename, 0, sizeof(m_filename));

  // If there is a temporary file, delete it.
  if (strlen(m_filenametmp) != 0) remove(m_filenametmp);

  memset(m_filenametmp, 0, sizeof(m_filenametmp));
}

// Check if the file is opened
bool CFile::IsOpened()
{
  if (m_fp == 0) return false; // Check for null pointer.

  return true;
}

// Close the file pointer and remove the file
bool CFile::CloseAndRemove()
{
  if (m_fp == 0) return true; // Check for null pointer.

  fclose(m_fp); // Close the file pointer

  m_fp = 0;

  if (remove(m_filename) != 0) { memset(m_filename, 0, sizeof(m_filename)); return false; }

  memset(m_filename, 0, sizeof(m_filename));
  memset(m_filenametmp, 0, sizeof(m_filenametmp));

  return true;
}

CFile::~CFile() // Class destructor
{
  Close();
}

// Open the file, parameters are the same as FOPEN, returns true on success, false on failure
bool CFile::Open(const char *filename, const char *openmode, bool bEnBuffer)
{
  Close();

  if ((m_fp = FOPEN(filename, openmode)) == 0) return false;

  memset(m_filename, 0, sizeof(m_filename));

  STRNCPY(m_filename, sizeof(m_filename), filename, 300);

  m_bEnBuffer = bEnBuffer;

  return true;
}

// Open the file for renaming, parameters are the same as fopen, returns true on success, false on failure
bool CFile::OpenForRename(const char *filename, const char *openmode, bool bEnBuffer)
{
  Close();

  memset(m_filename, 0, sizeof(m_filename));
  STRNCPY(m_filename, sizeof(m_filename), filename, 300);

  memset(m_filenametmp, 0, sizeof(m_filenametmp));
  SNPRINTF(m_filenametmp, sizeof(m_filenametmp), 300, "%s.tmp", m_filename);

  if ((m_fp = FOPEN(m_filenametmp, openmode)) == 0) return false;

  m_bEnBuffer = bEnBuffer;

  return true;
}

// Close the file and rename it
bool CFile::CloseAndRename()
{
  if (m_fp == 0) return false; // Check for null pointer.

  fclose(m_fp); // Close the file pointer

  m_fp = 0;

  if (rename(m_filenametmp, m_filename) != 0)
  {
    remove(m_filenametmp);
    memset(m_filename, 0, sizeof(m_filename));
    memset(m_filenametmp, 0, sizeof(m_filenametmp));
    return false;
  }

  memset(m_filename, 0, sizeof(m_filename));
  memset(m_filenametmp, 0, sizeof(m_filenametmp));

  return true;
}


// Call fprintf to write data to the file
void CFile::Fprintf(const char *fmt, ...)
{
  if (m_fp == 0) return;

  va_list arg;
  va_start(arg, fmt);
  vfprintf(m_fp, fmt, arg);
  va_end(arg);

  if (m_bEnBuffer == false) fflush(m_fp);
}

// Call fgets to read a line from the file, bDelCRT=true to remove newline characters, false otherwise (default is false)
bool CFile::Fgets(char *buffer, const int readsize, bool bdelcrt)
{
  if (m_fp == 0) return false;

  memset(buffer, 0, readsize + 1); // The caller must ensure that buffer has enough space, otherwise, there may be a memory overflow.

  if (fgets(buffer, readsize, m_fp) == 0) return false;

  if (bdelcrt == true)
  {
    DeleteRChar(buffer, '\n');
    DeleteRChar(buffer, '\r'); // If the file is in Windows format, remove '\r'.
  }

  return true;
}

// Read a line from the file
// strEndStr is the end-of-line marker for a line of data, if empty, "\n" is used as the end-of-line marker.
bool CFile::FFGETS(char *buffer, const int readsize, const char *endbz)
{
  if (m_fp == 0) return false;

  return FGETS(m_fp, buffer, readsize, endbz);
}

// Call fread to read data from the file.
size_t CFile::Fread(void *ptr, size_t size)
{
  if (m_fp == 0) return -1;

  return fread(ptr, 1, size, m_fp);
}

// Call fwrite to write data to the file
size_t CFile::Fwrite(const void *ptr, size_t size)
{
  if (m_fp == 0) return -1;

  size_t tt = fwrite(ptr, 1, size, m_fp);

  if (m_bEnBuffer == false) fflush(m_fp);

  return tt;
}


// Read a line from a text file.
// fp: the file pointer that is already opened.
// buffer: used to store the read content.
// readsize: the number of bytes to be read in this operation. If the end marker is reached, the function returns.
// endbz: the end marker for the line content. The default value is empty, indicating that the line content ends with "\n".
// Return value: true on success; false on failure, in general, failure can be regarded as the end of the file.
bool FGETS(const FILE *fp, char *buffer, const int readsize, const char *endbz)
{
  if (fp == 0) return false;

  memset(buffer, 0, readsize + 1); // The caller must ensure that buffer has enough space, otherwise, there may be a memory overflow.

  char strline[readsize + 1];

  while (true)
  {
    memset(strline, 0, sizeof(strline));

    if (fgets(strline, readsize, (FILE *)fp) == 0) break;

    // Prevent buffer overflow
    if ((strlen(buffer) + strlen(strline)) >= (unsigned int)readsize) break;

    strcat(buffer, strline);

    if (endbz == 0) return true;
    else if (strstr(strline, endbz) != 0) return true;
    // The above code can be optimized to compare only the last part of the content without using strstr.
  }

  return false;
}


CCmdStr::CCmdStr()
{
  m_vCmdStr.clear();
}

CCmdStr::CCmdStr(const string &buffer, const char *sepstr, const bool bdelspace)
{
  m_vCmdStr.clear();

  SplitToCmd(buffer, sepstr, bdelspace);
}

// Split the string into m_vCmdStr container.
// buffer: the string to be split.
// sepstr: the separator for the fields in the buffer string. Note that the separator is a string, such as ",", " ", "|", "~!~".
// bdelspace: whether to remove leading and trailing spaces of the split fields, true-remove; false-do not remove, the default is not removed.
void CCmdStr::SplitToCmd(const string &buffer, const char *sepstr, const bool bdelspace)
{
  // Clear all old data
  m_vCmdStr.clear();

  int iPOS = 0;
  string srcstr, substr;

  srcstr = buffer;

  while ((iPOS = srcstr.find(sepstr)) >= 0)
  {
    substr = srcstr.substr(0, iPOS);

    if (bdelspace == true)
    {
      char str[substr.length() + 1];
      STRCPY(str, sizeof(str), substr.c_str());

      DeleteLRChar(str, ' ');

      substr = str;
    }

    m_vCmdStr.push_back(substr);

    iPOS = iPOS + strlen(sepstr);

    srcstr = srcstr.substr(iPOS, srcstr.size() - iPOS);
  }

  substr = srcstr;

  if (bdelspace == true)
  {
    char str[substr.length() + 1];
    STRCPY(str, sizeof(str), substr.c_str());

    DeleteLRChar(str, ' ');

    substr = str;
  }

  m_vCmdStr.push_back(substr);

  return;
}


int CCmdStr::CmdCount()
{
  return m_vCmdStr.size();
}

bool CCmdStr::GetValue(const int inum,char *value,const int ilen)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  if (ilen>0) memset(value,0,ilen+1);   // 调用者必须保证value的空间足够，否则这里会内存溢出。

  if ( (m_vCmdStr[inum].length()<=(unsigned int)ilen) || (ilen==0) )
  {
    strcpy(value,m_vCmdStr[inum].c_str());
  }
  else
  {
    strncpy(value,m_vCmdStr[inum].c_str(),ilen); value[ilen]=0;
  }

  return true;
}

bool CCmdStr::GetValue(const int inum,int *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  (*value) = atoi(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,unsigned int *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  (*value) = atoi(m_vCmdStr[inum].c_str());

  return true;
}


bool CCmdStr::GetValue(const int inum,long *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  (*value) = atol(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,unsigned long *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  (*value) = atol(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,double *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  (*value) = (double)atof(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,bool *value)
{
  if ( (inum>=(int)m_vCmdStr.size()) || (value==0) ) return false;

  (*value) = 0;

  if (inum >= (int)m_vCmdStr.size()) return false;

  char strTemp[11];
  memset(strTemp,0,sizeof(strTemp));
  strncpy(strTemp,m_vCmdStr[inum].c_str(),10);

  ToUpper(strTemp);  
  if (strcmp(strTemp,"TRUE")==0) (*value)=true; 

  return true;
}

CCmdStr::~CCmdStr()
{
  m_vCmdStr.clear();
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,char *value,const int ilen)
{
  if (value==0) return false;    

  if (ilen>0) memset(value,0,ilen+1); // The caller must ensure that 'value' has enough space, otherwise there may be a memory overflow.

  char *start=0,*end=0;
  char m_SFieldName[51],m_EFieldName[51];

  int m_NameLen = strlen(fieldname);

  SNPRINTF(m_SFieldName,sizeof(m_SFieldName),50,"<%s>",fieldname);
  SNPRINTF(m_EFieldName,sizeof(m_EFieldName),50,"</%s>",fieldname);

  start=0; end=0;

  start = (char *)strstr(xmlbuffer,m_SFieldName);

  if (start != 0)
  {
    end   = (char *)strstr(start,m_EFieldName);
  }

  if ((start==0) || (end == 0))
  {
    return false;
  }

  int   m_ValueLen = end - start - m_NameLen - 2 ;

  if ( ((m_ValueLen) <= ilen) || (ilen == 0) )
  {
    strncpy(value,start+m_NameLen+2,m_ValueLen); value[m_ValueLen]=0;
  }
  else
  {
    strncpy(value,start+m_NameLen+2,ilen); value[ilen]=0;
  }

  DeleteLRChar(value,' ');

  return true;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,bool *value)
{
  if (value==0) return false;    

  (*value) = false;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,10) == true)
  {
    ToUpper(strTemp);  
    if (strcmp(strTemp,"TRUE")==0) { (*value)=true; return true; }
  }

  return false;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,int *value)
{
  if (value==0) return false;    

  (*value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,50) == true)
  {
    (*value) = atoi(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,unsigned int *value)
{
  if (value==0) return false;    

  (*value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,50) == true)
  {
    (*value) = (unsigned int)atoi(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,long *value)
{
  if (value==0) return false;    

  (*value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,50) == true)
  {
    (*value) = atol(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,unsigned long *value)
{
  if (value==0) return false;    

  (*value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,50) == true)
  {
    (*value) = (unsigned long)atol(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *xmlbuffer,const char *fieldname,double *value)
{
  if (value==0) return false;    

  (*value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(xmlbuffer,fieldname,strTemp,50) == true)
  {
    (*value) = atof(strTemp); return true;
  }

  return false;
}

/ Convert the integer representation of time to the string representation of time.
// ltime: Integer representation of time.
// stime: String representation of time.
// fmt: The format of the output string time 'stime', which is the same as the 'fmt' parameter of the LocalTime function. If the format of 'fmt' is incorrect, 'stime' will be empty.
void timetostr(const time_t ltime,char *stime,const char *fmt)
{
  if (stime==0) return;    

  strcpy(stime,"");

  struct tm sttm = *localtime ( &ltime );
  // struct tm sttm; localtime_r(&ltime,&sttm); 

  sttm.tm_year=sttm.tm_year+1900;
  sttm.tm_mon++;

  if (fmt==0)
  {
    snprintf(stime,20,"%04u-%02u-%02u %02u:%02u:%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour,
                    sttm.tm_min,sttm.tm_sec);
    return;
  }

  if (strcmp(fmt,"yyyy-mm-dd hh24:mi:ss") == 0)
  {
    snprintf(stime,20,"%04u-%02u-%02u %02u:%02u:%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour,
                    sttm.tm_min,sttm.tm_sec);
    return;
  }

  if (strcmp(fmt,"yyyy-mm-dd hh24:mi") == 0)
  {
    snprintf(stime,17,"%04u-%02u-%02u %02u:%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour,
                    sttm.tm_min);
    return;
  }

  if (strcmp(fmt,"yyyy-mm-dd hh24") == 0)
  {
    snprintf(stime,14,"%04u-%02u-%02u %02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour);
    return;
  }

  if (strcmp(fmt,"yyyy-mm-dd") == 0)
  {
    snprintf(stime,11,"%04u-%02u-%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday); 
    return;
  }

  if (strcmp(fmt,"yyyy-mm") == 0)
  {
    snprintf(stime,8,"%04u-%02u",sttm.tm_year,sttm.tm_mon); 
    return;
  }

  if (strcmp(fmt,"yyyymmddhh24miss") == 0)
  {
    snprintf(stime,15,"%04u%02u%02u%02u%02u%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour,
                    sttm.tm_min,sttm.tm_sec);
    return;
  }

  if (strcmp(fmt,"yyyymmddhh24mi") == 0)
  {
    snprintf(stime,13,"%04u%02u%02u%02u%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour,
                    sttm.tm_min);
    return;
  }

  if (strcmp(fmt,"yyyymmddhh24") == 0)
  {
    snprintf(stime,11,"%04u%02u%02u%02u",sttm.tm_year,
                    sttm.tm_mon,sttm.tm_mday,sttm.tm_hour);
    return;
  }

  if (strcmp(fmt,"yyyymmdd") == 0)
  {
    snprintf(stime,9,"%04u%02u%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday); 
    return;
  }

  if (strcmp(fmt,"hh24miss") == 0)
  {
    snprintf(stime,7,"%02u%02u%02u",sttm.tm_hour,sttm.tm_min,sttm.tm_sec); 
    return;
  }

  if (strcmp(fmt,"hh24mi") == 0)
  {
    snprintf(stime,5,"%02u%02u",sttm.tm_hour,sttm.tm_min); 
    return;
  }

  if (strcmp(fmt,"hh24") == 0)
  {
    snprintf(stime,3,"%02u",sttm.tm_hour); 
    return;
  }

  if (strcmp(fmt,"mi") == 0)
  {
    snprintf(stime,3,"%02u",sttm.tm_min); 
    return;
  }
}


/*
Get the time from the operating system and convert the integer representation of time to the string representation format.
stime: Used to store the obtained time string.
timetvl: Time offset, in seconds. The default value is 0, which represents the current time. timetvl=30 means the time 30 seconds after the current time, and timetvl=-30 means the time 30 seconds before the current time.
fmt: The output format of the time. The default format is "yyyy-mm-dd hh24:mi:ss", and it currently supports the following formats:
"yyyy-mm-dd hh24:mi:ss" - This format is the default format.
"yyyymmddhh24miss"
"yyyy-mm-dd"
"yyyymmdd"
"hh24:mi:ss"
"hh24miss"
"hh24:mi"
"hh24mi"
"hh24"
"mi"
Note:
1) The representation for hours is "hh24", not "hh". This is done to maintain consistency with the time representation in the database.
2) The above list includes commonly used time formats. If they do not meet the requirements of your application development, modify the source code to add support for more formats.
3) When calling the function, if 'fmt' matches any of the above formats, the content of 'stime' will be empty.
*/
void LocalTime(char *stime,const char *fmt,const int timetvl)
{
  if (stime==0) return;    

  time_t  timer;

  time( &timer ); timer=timer+timetvl;

  timetostr(timer,stime,fmt);
}


CLogFile::CLogFile(const long MaxLogSize)
{
  m_tracefp = 0;
  memset(m_filename,0,sizeof(m_filename));
  memset(m_openmode,0,sizeof(m_openmode));
  m_bBackup=true;
  m_bEnBuffer=false;
  m_MaxLogSize=MaxLogSize;
  if (m_MaxLogSize<10) m_MaxLogSize=10;

  // pthread_pin_init(&spin,0);  
}

CLogFile::~CLogFile()
{
  Close();

  // pthread_spin_destroy(&spin);  
}

void CLogFile::Close()
{
  if (m_tracefp != 0) { fclose(m_tracefp); m_tracefp=0; }

  memset(m_filename,0,sizeof(m_filename));
  memset(m_openmode,0,sizeof(m_openmode));
  m_bBackup=true;
  m_bEnBuffer=false;
}

/ Open the log file.
// filename: The name of the log file. It is recommended to use an absolute path. If the directory in the filename does not exist, create the directory first.
// openmode: The mode to open the log file, same as the fopen library function. The default value is "a+".
// bBackup: Whether to enable automatic switching of log files. true - enable, false - disable. In multi-process service programs, if multiple processes share the same log file, bBackup must be set to false.
// bEnBuffer: Whether to enable file buffering mechanism. true - enable, false - disable. If the buffer is enabled, the contents written to the log file will not be immediately written to the file. The default is to disable buffering.
bool CLogFile::Open(const char *filename,const char *openmode,bool bBackup,bool bEnBuffer)
{
  Close();

  STRCPY(m_filename,sizeof(m_filename),filename);
  m_bEnBuffer=bEnBuffer;
  m_bBackup=bBackup;
  if (openmode==0) STRCPY(m_openmode,sizeof(m_openmode),"a+");
  else STRCPY(m_openmode,sizeof(m_openmode),openmode);

  if ((m_tracefp=FOPEN(m_filename,m_openmode)) == 0) return false;

  return true;
}

// If the log file size exceeds 100MB, back up the current log file as a historical log file and clear the current log file's content after successful backup.
// The backed-up file will have the date and time appended to the log file name.
// Note: In multi-process programs, the log file cannot be switched, while in multi-threaded programs, the log file can be switched.
bool CLogFile::BackupLogFile()
{
  if (m_tracefp == 0) return false;

  // Do not backup
  if (m_bBackup == false) return true;

  //fseek(m_tracefp,0,2);

  if (ftell(m_tracefp) > m_MaxLogSize * 1024 * 1024)
  {
    fclose(m_tracefp);
    m_tracefp = 0;

    char strLocalTime[21];
    memset(strLocalTime, 0, sizeof(strLocalTime));
    LocalTime(strLocalTime, "yyyymmddhh24miss");

    char bak_filename[301];
    SNPRINTF(bak_filename, sizeof(bak_filename), 300, "%s.%s", m_filename, strLocalTime);
    rename(m_filename, bak_filename);

    if ((m_tracefp = FOPEN(m_filename, m_openmode)) == 0) return false;
  }

  return true;
}

// Write the content to the log file. 'fmt' is a variable argument, and it is used in the same way as the printf library function.
// The Write method will write the current time, while the WriteEx method will not write the time.
bool CLogFile::Write(const char *fmt, ...)
{
  if (m_tracefp == 0) return false;

  // pthread_spin_lock(&spin);  

  if (BackupLogFile() == false) return false;

  char strtime[20]; LocalTime(strtime);
  va_list ap;
  va_start(ap, fmt);
  fprintf(m_tracefp, "%s ", strtime);
  vfprintf(m_tracefp, fmt, ap);
  va_end(ap);

  if (m_bEnBuffer == false) fflush(m_tracefp);

  // pthread_spin_unlock(&spin);  

  return true;
}

// Write the content to the log file. 'fmt' is a variable argument, and it is used in the same way as the printf library function.
// The Write method will write the current time, while the WriteEx method will not write the time.
bool CLogFile::WriteEx(const char *fmt, ...)
{
  if (m_tracefp == 0) return false;

  // pthread_spin_lock(&spin);  

  va_list ap;
  va_start(ap, fmt);
  vfprintf(m_tracefp, fmt, ap);
  va_end(ap);

  if (m_bEnBuffer == false) fflush(m_tracefp);

  // pthread_spin_unlock(&spin);  

  return true;
}


CIniFile::CIniFile()
{
  
}

bool CIniFile::LoadFile(const char *filename)
{
  m_xmlbuffer.clear();

  CFile File;

  if ( File.Open(filename,"r") == false) return false;

  char strLine[501];

  while (true)
  {
    memset(strLine,0,sizeof(strLine));

    if (File.FFGETS(strLine,500) == false) break;

    m_xmlbuffer=m_xmlbuffer+strLine;
  }

  if (m_xmlbuffer.length() < 10) return false;

  return true;
}

bool CIniFile::GetValue(const char *fieldname,bool   *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

bool CIniFile::GetValue(const char *fieldname,char *value,int ilen)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value,ilen);
}

bool CIniFile::GetValue(const char *fieldname,int *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

bool CIniFile::GetValue(const char *fieldname,unsigned int *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

bool CIniFile::GetValue(const char *fieldname,long *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

bool CIniFile::GetValue(const char *fieldname,unsigned long *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

bool CIniFile::GetValue(const char *fieldname,double *value)
{
  return GetXMLBuffer(m_xmlbuffer.c_str(),fieldname,value);
}

// Close all IO and Signal
void CloseIOAndSignal(bool bCloseIO)
{
  int ii=0;

  for (ii=0;ii<64;ii++)
  {
    if (bCloseIO==true) close(ii);

    signal(ii,SIG_IGN); 
  }
}

// Create directories step by step based on the absolute path of a file or directory name.
// pathorfilename: Absolute path of a file name or directory name.
// bisfilename: Indicates the type of pathorfilename. true - pathorfilename is a file name, otherwise it is a directory name, default is true.
// Return value: true - creation success, false - creation failure. If the function returns false, there are generally three possible reasons: 1) insufficient permission; 2) pathorfilename is not a valid file name or directory name; 3) insufficient disk space.
bool MKDIR(const char *filename, bool bisfilename)
{
  // Check if the directory exists, if not, create subdirectories step by step.
  char strPathName[301];

  int ilen = strlen(filename);

  for (int ii = 1; ii < ilen; ii++)
  {
    if (filename[ii] != '/') continue;

    STRNCPY(strPathName, sizeof(strPathName), filename, ii);

    if (access(strPathName, F_OK) == 0) continue; // If the directory already exists, continue.

    if (mkdir(strPathName, 0755) != 0) return false; // If the directory does not exist, create it.
  }

  if (bisfilename == false)
  {
    if (access(filename, F_OK) != 0)
    {
      if (mkdir(filename, 0755) != 0) return false;
    }
  }

  return true;
}

// Open a file.
// The FOPEN function calls the fopen library function to open a file, and if the directory in the file name does not exist, it creates the directory.
// The parameters and return value of the FOPEN function are exactly the same as the fopen function.
// In application development, use the FOPEN function instead of the fopen library function.
FILE *FOPEN(const char *filename, const char *mode)
{
  if (MKDIR(filename) == false) return 0;

  return fopen(filename, mode);
}

// Get the size of a file.
// filename: The name of the file to get the size of, it is recommended to use the absolute path of the file name.
// Return value: If the file does not exist or does not have access permission, return -1. On success, return the size of the file in bytes.
int FileSize(const char *filename)
{
  struct stat st_filestat;

  if (stat(filename, &st_filestat) < 0) return -1;

  return st_filestat.st_size;
}

// Reset the modification time attribute of a file.
// filename: The name of the file to reset the modification time, it is recommended to use the absolute path of the file name.
// mtime: String representation of time, the format is not limited, but it must include yyyymmddhh24miss, all components are required.
// Return value: true - success; false - failure, the reason for failure is stored in errno.
bool UTime(const char *filename, const char *mtime)
{
  struct utimbuf stutimbuf;

  stutimbuf.actime = stutimbuf.modtime = strtotime(mtime);

  if (utime(filename, &stutimbuf) != 0) return false;

  return true;
}


// Convert a string representation of time to integer representation of time.
// stime: String representation of time, the format is not limited, but it must include yyyymmddhh24miss, all components are required.
// Return value: Integer representation of time, if the format of stime is incorrect, return -1.
time_t strtotime(const char *stime)
{
  char strtime[21], yyyy[5], mm[3], dd[3], hh[3], mi[3], ss[3];
  memset(strtime, 0, sizeof(strtime));
  memset(yyyy, 0, sizeof(yyyy));
  memset(mm, 0, sizeof(mm));
  memset(dd, 0, sizeof(dd));
  memset(hh, 0, sizeof(hh));
  memset(mi, 0, sizeof(mi));
  memset(ss, 0, sizeof(ss));

  PickNumber(stime, strtime, false, false);

  if (strlen(strtime) != 14) return -1;

  strncpy(yyyy, strtime, 4);
  strncpy(mm, strtime + 4, 2);
  strncpy(dd, strtime + 6, 2);
  strncpy(hh, strtime + 8, 2);
  strncpy(mi, strtime + 10, 2);
  strncpy(ss, strtime + 12, 2);

  struct tm time_str;

  time_str.tm_year = atoi(yyyy) - 1900;
  time_str.tm_mon = atoi(mm) - 1;
  time_str.tm_mday = atoi(dd);
  time_str.tm_hour = atoi(hh);
  time_str.tm_min = atoi(mi);
  time_str.tm_sec = atoi(ss);
  time_str.tm_isdst = 0;

  return mktime(&time_str);
}

// Add a number of seconds to the time represented by a string and obtain a new string representation of time.
// in_stime: Input string representation of time.
// out_stime: Output string representation of time.
// timetvl: Number of seconds to offset, positive for future, negative for past.
// fmt: Output format of the string time out_stime, same as the fmt parameter of the LocalTime function.
// Note: The in_stime and out_stime parameters can be the addresses of the same variable. If the call fails, the content of out_stime will be cleared.
// Return value: true - success, false - failure. If the function returns false, it is likely that the format of in_stime is incorrect.
bool AddTime(const char *in_stime, char *out_stime, const int timetvl, const char *fmt)
{
  if ((in_stime == 0) || (out_stime == 0)) return false; // Check for null pointers.

  time_t timer;
  if ((timer = strtotime(in_stime)) == -1) { strcpy(out_stime, ""); return false; }

  timer = timer + timetvl;

  strcpy(out_stime, "");

  timetostr(timer, out_stime, fmt);

  return true;
}

// Get the time of a file.
// filename: The name of the file to get the time of, it is recommended to use the absolute path of the file name.
// mtime: Used to store the time of the file, which is st_mtime of the stat structure.
// fmt: Set the output format of the time, same as the LocalTime function, but the default is "yyyymmddhh24miss".
// Return value: If the file does not exist or does not have access permission, return false. On success, return true.
bool FileMTime(const char *filename, char *mtime, const char *fmt)
{
  // Check if the file exists.
  struct stat st_filestat;

  if (stat(filename, &st_filestat) < 0) return false;

  char strfmt[25];
  memset(strfmt, 0, sizeof(strfmt));
  if (fmt == 0) strcpy(strfmt, "yyyymmddhh24miss");
  else strcpy(strfmt, fmt);

  timetostr(st_filestat.st_mtime, mtime, strfmt);

  return true;
}

CDir::CDir()
{
  m_pos = 0;

  STRCPY(m_DateFMT, sizeof(m_DateFMT), "yyyy-mm-dd hh24:mi:ss");

  m_vFileName.clear();

  initdata();
}

void CDir::initdata()
{
  memset(m_DirName, 0, sizeof(m_DirName));
  memset(m_FileName, 0, sizeof(m_FileName));
  memset(m_FullFileName, 0, sizeof(m_FullFileName));
  m_FileSize = 0;
  memset(m_CreateTime, 0, sizeof(m_CreateTime));
  memset(m_ModifyTime, 0, sizeof(m_ModifyTime));
  memset(m_AccessTime, 0, sizeof(m_AccessTime));
}

// Set the date format of the file time, support "yyyy-mm-dd hh24:mi:ss" and "yyyymmddhh24miss", the default is the former.
void CDir::SetDateFMT(const char *in_DateFMT)
{
  memset(m_DateFMT, 0, sizeof(m_DateFMT));
  STRCPY(m_DateFMT, sizeof(m_DateFMT), in_DateFMT);
}

// Open a directory, get the file list information in the directory, and store it in the m_vFileName container.
// in_DirName: The directory name to open.
// in_MatchStr: The matching rule for getting file names, files that do not match are ignored.
// in_MaxCount: The maximum number of files to get, default is 10000.
// bAndChild: Whether to open subdirectories, default is false - do not open subdirectories.
// bSort: Whether to sort the obtained file list (i.e., the content of the m_vFileName container), default is false - do not sort.
// Return value: If the directory specified by the in_DirName parameter does not exist, the OpenDir method will create the directory. If the creation fails, return false. Also, if the current user does not have read permission for the subdirectories under in_DirName, it will also return false. In all other normal cases, it will return true.
bool CDir::OpenDir(const char *in_DirName, const char *in_MatchStr, const unsigned int in_MaxCount, const bool bAndChild, bool bSort)
{
  m_pos = 0;
  m_vFileName.clear();

  // If the directory does not exist, create it.
  if (MKDIR(in_DirName, false) == false) return false;

  bool bRet = _OpenDir(in_DirName, in_MatchStr, in_MaxCount, bAndChild);

  if (bSort == true)
  {
    sort(m_vFileName.begin(), m_vFileName.end());
  }

  return bRet;
}


// This is a recursive function used for the internal call of OpenDir(). It does not need to be called outside the CDir class.
bool CDir::_OpenDir(const char *in_DirName, const char *in_MatchStr, const unsigned int in_MaxCount, const bool bAndChild)
{
  DIR *dir;

  if ((dir = opendir(in_DirName)) == 0) return false;

  char strTempFileName[3001];

  struct dirent *st_fileinfo;
  struct stat st_filestat;

  while ((st_fileinfo = readdir(dir)) != 0)
  {
    // Files starting with "." are not processed
    if (st_fileinfo->d_name[0] == '.') continue;

    SNPRINTF(strTempFileName, sizeof(strTempFileName), 300, "%s/%s", in_DirName, st_fileinfo->d_name);

    UpdateStr(strTempFileName, "/", "/");

    stat(strTempFileName, &st_filestat);

    // Check if it is a directory, if yes, process subdirectories.
    if (S_ISDIR(st_filestat.st_mode))
    {
      if (bAndChild == true)
      {
        if (_OpenDir(strTempFileName, in_MatchStr, in_MaxCount, bAndChild) == false)
        {
          closedir(dir);
          return false;
        }
      }
    }
    else
    {
      // If it is a file, put the matching files into the m_vFileName container.
      if (MatchStr(st_fileinfo->d_name, in_MatchStr) == false) continue;

      m_vFileName.push_back(strTempFileName);

      if (m_vFileName.size() >= in_MaxCount) break;
    }
  }

  closedir(dir);

  return true;
}


/*
st_gid 
  Numeric identifier of group that owns file (UNIX-specific) This field will always be zero on NT systems. A redirected file is classified as an NT file.
st_atime
  Time of last access of file.
st_ctime
  Time of creation of file.
st_dev
  Drive number of the disk containing the file (same as st_rdev).
st_ino
  Number of the information node (the inode) for the file (UNIX-specific). On UNIX file systems, the inode describes the file date and time stamps, permissions, and content. When files are hard-linked to one another, they share the same inode. The inode, and therefore st_ino, has no meaning in the FAT, HPFS, or NTFS file systems.
st_mode
  Bit mask for file-mode information. The _S_IFDIR bit is set if path specifies a directory; the _S_IFREG bit is set if path specifies an ordinary file or a device. User read/write bits are set according to the file’s permission mode; user execute bits are set according to the filename extension.
st_mtime
  Time of last modification of file.
st_nlink
  Always 1 on non-NTFS file systems.
st_rdev
  Drive number of the disk containing the file (same as st_dev).
st_size
  Size of the file in bytes; a 64-bit integer for _stati64 and _wstati64
st_uid
  Numeric identifier of user who owns file (UNIX-specific). This field will always be zero on NT systems. A redirected file is classified as an NT file.
*/

// Get a record (file name) from the m_vFileName container and retrieve information about the file, such as size and modification time.
// When calling the OpenDir method, the m_vFileName container is cleared, and m_pos is set to zero. Each call to ReadDir method increments m_pos by 1.
// If m_pos is less than m_vFileName.size(), it returns true, otherwise, it returns false.
bool CDir::ReadDir()
{
  initdata();

  int ivsize = m_vFileName.size();

  // If already read all records, clear the container
  if (m_pos >= ivsize)
  {
    m_pos = 0;
    m_vFileName.clear();
    return false;
  }

  int pos = 0;

  pos = m_vFileName[m_pos].find_last_of("/");

  // Directory name
  STRCPY(m_DirName, sizeof(m_DirName), m_vFileName[m_pos].substr(0, pos).c_str());

  // File name
  STRCPY(m_FileName, sizeof(m_FileName), m_vFileName[m_pos].substr(pos + 1, m_vFileName[m_pos].size() - pos - 1).c_str());

  // Full file name, including path
  SNPRINTF(m_FullFileName, sizeof(m_FullFileName), 300, "%s", m_vFileName[m_pos].c_str());

  struct stat st_filestat;

  stat(m_FullFileName, &st_filestat);

  m_FileSize = st_filestat.st_size;

  struct tm nowtimer;

  if (strcmp(m_DateFMT, "yyyy-mm-dd hh24:mi:ss") == 0)
  {
    nowtimer = *localtime(&st_filestat.st_mtime);
    nowtimer.tm_mon++;
    snprintf(m_ModifyTime, 20, "%04u-%02u-%02u %02u:%02u:%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_ctime);
    nowtimer.tm_mon++;
    snprintf(m_CreateTime, 20, "%04u-%02u-%02u %02u:%02u:%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_atime);
    nowtimer.tm_mon++;
    snprintf(m_AccessTime, 20, "%04u-%02u-%02u %02u:%02u:%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);
  }

  if (strcmp(m_DateFMT, "yyyymmddhh24miss") == 0)
  {
    nowtimer = *localtime(&st_filestat.st_mtime);
    nowtimer.tm_mon++;
    snprintf(m_ModifyTime, 20, "%04u%02u%02u%02u%02u%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_ctime);
    nowtimer.tm_mon++;
    snprintf(m_CreateTime, 20, "%04u%02u%02u%02u%02u%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_atime);
    nowtimer.tm_mon++;
    snprintf(m_AccessTime, 20, "%04u%02u%02u%02u%02u%02u",
             nowtimer.tm_year + 1900, nowtimer.tm_mon, nowtimer.tm_mday,
             nowtimer.tm_hour, nowtimer.tm_min, nowtimer.tm_sec);
  }

  m_pos++;

  return true;
}


CDir::~CDir()
{
  m_vFileName.clear();
  // m_vDirName.clear();
}

bool REMOVE(const char *filename, const int times)
{
  if (access(filename, R_OK) != 0) return false;

  for (int ii = 0; ii < times; ii++)
  {
    if (remove(filename) == 0) return true;
    usleep(100000);
  }

  return false;
}

bool RENAME(const char *srcfilename, const char *dstfilename, const int times)
{
  if (access(srcfilename, R_OK) != 0) return false;

  if (MKDIR(dstfilename) == false) return false;

  for (int ii = 0; ii < times; ii++)
  {
    if (rename(srcfilename, dstfilename) == 0) return true;
    usleep(100000);
  }

  return false;
}

CTcpClient::CTcpClient()
{
  m_connfd = -1;
  memset(m_ip, 0, sizeof(m_ip));
  m_port = 0;
  m_btimeout = false;
}

bool CTcpClient::ConnectToServer(const char *ip, const int port)
{
  if (m_connfd != -1)
  {
    close(m_connfd);
    m_connfd = -1;
  }

  signal(SIGPIPE, SIG_IGN);

  STRCPY(m_ip, sizeof(m_ip), ip);
  m_port = port;

  struct hostent *h;
  struct sockaddr_in servaddr;

  if ((m_connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return false;

  if (!(h = gethostbyname(m_ip)))
  {
    close(m_connfd);
    m_connfd = -1;
    return false;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(m_port);
  memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

  if (connect(m_connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
  {
    close(m_connfd);
    m_connfd = -1;
    return false;
  }

  return true;
}


bool CTcpClient::Read(char *buffer, const int itimeout)
{
  if (m_connfd == -1) return false;

  if (itimeout > 0)
  {
    struct pollfd fds;
    fds.fd = m_connfd;
    fds.events = POLLIN;
    int iret;
    m_btimeout = false;
    if ((iret = poll(&fds, 1, itimeout * 1000)) <= 0)
    {
      if (iret == 0) m_btimeout = true;
      return false;
    }
  }

  m_buflen = 0;
  return (TcpRead(m_connfd, buffer, &m_buflen));
}

bool CTcpClient::Write(const char *buffer, const int ibuflen)
{
  if (m_connfd == -1) return false;

  int ilen = ibuflen;

  if (ibuflen == 0) ilen = strlen(buffer);

  return (TcpWrite(m_connfd, buffer, ilen));
}

void CTcpClient::Close()
{
  if (m_connfd > 0) close(m_connfd);

  m_connfd = -1;
  memset(m_ip, 0, sizeof(m_ip));
  m_port = 0;
  m_btimeout = false;
}

CTcpClient::~CTcpClient()
{
  Close();
}

CTcpServer::CTcpServer()
{
  m_listenfd = -1;
  m_connfd = -1;
  m_socklen = 0;
  m_btimeout = false;
}

bool CTcpServer::InitServer(const unsigned int port, const int backlog)
{
  if (m_listenfd > 0) { close(m_listenfd); m_listenfd = -1; }

  if ((m_listenfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) return false;

  signal(SIGPIPE, SIG_IGN);

  int opt = 1;
  unsigned int len = sizeof(opt);
  setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);

  memset(&m_servaddr, 0, sizeof(m_servaddr));
  m_servaddr.sin_family = AF_INET;
  m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  m_servaddr.sin_port = htons(port);
  if (bind(m_listenfd, (struct sockaddr *)&m_servaddr, sizeof(m_servaddr)) != 0)
  {
    CloseListen(); return false;
  }

  if (listen(m_listenfd, backlog) != 0)
  {
    CloseListen(); return false;
  }

  return true;
}

bool CTcpServer::Accept()
{
  if (m_listenfd == -1) return false;

  m_socklen = sizeof(struct sockaddr_in);

  if ((m_connfd = accept(m_listenfd, (struct sockaddr *)&m_clientaddr, (socklen_t *)&m_socklen)) < 0)
    return false;

  return true;
}

char *CTcpServer::GetIP()
{
  return (inet_ntoa(m_clientaddr.sin_addr));
}


bool CTcpServer::Read(char *buffer, const int itimeout)
{
  if (m_connfd == -1) return false;

  // If itimeout > 0, it means we need to wait for itimeout seconds. If there's no data
  // arriving after itimeout seconds, return false.
  if (itimeout > 0)
  {
    struct pollfd fds;
    fds.fd = m_connfd;
    fds.events = POLLIN;
    m_btimeout = false;
    int iret;
    if ((iret = poll(&fds, 1, itimeout * 1000)) <= 0)
    {
      if (iret == 0) m_btimeout = true;
      return false;
    }
  }

  m_buflen = 0;
  return TcpRead(m_connfd, buffer, &m_buflen);
}

bool CTcpServer::Write(const char *buffer, const int ibuflen)
{
  if (m_connfd == -1) return false;

  int ilen = ibuflen;
  if (ilen == 0) ilen = strlen(buffer);

  return TcpWrite(m_connfd, buffer, ilen);
}

void CTcpServer::CloseListen()
{
  // If the server's socket (m_listenfd) is greater than 0, close it.
  if (m_listenfd > 0)
  {
    close(m_listenfd); m_listenfd = -1;
  }
}

void CTcpServer::CloseClient()
{
  // If the client's socket (m_connfd) is greater than 0, close it.
  if (m_connfd > 0)
  {
    close(m_connfd); m_connfd = -1;
  }
}

CTcpServer::~CTcpServer()
{
  // Close both the listening socket and the client socket on destruction.
  CloseListen();
  CloseClient();
}

// Receive data sent by the remote end of the socket (server-side).
// sockfd: The valid socket connection.
// buffer: The address of the data reception buffer.
// ibuflen: The number of bytes of data received successfully in this call.
// itimeout: Timeout for data reception in seconds. -1: no waiting, 0: infinite waiting, >0: timeout in seconds.
// Return value: true - success; false - failure, which can be due to timeout or the socket connection being unavailable.
bool TcpRead(const int sockfd, char *buffer, int *ibuflen, const int itimeout)
{
  if (sockfd == -1) return false;

  // If itimeout > 0, it means we need to wait for itimeout seconds. If there's no data
  // arriving after itimeout seconds, return false.
  if (itimeout > 0)
  {
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    if (poll(&fds, 1, itimeout * 1000) <= 0) return false;
  }

  // If itimeout == -1, it means we don't wait and immediately check if there's data in the socket buffer. If there's no data, return false.
  if (itimeout == -1)
  {
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    if (poll(&fds, 1, 0) <= 0) return false;
  }

  (*ibuflen) = 0;

  // Read the message length, 4 bytes, first.
  if (Readn(sockfd, (char *)ibuflen, 4) == false) return false;

  (*ibuflen) = ntohl(*ibuflen);

  // Read the message content.
  if (Readn(sockfd, buffer, (*ibuflen)) == false) return false;

  return true;
}


// Send data to the remote end of the socket.
// sockfd: The valid socket connection.
// buffer: The address of the data to be sent.
// ibuflen: The number of bytes to be sent. If sending an ASCII string, set ibuflen to 0 or the length of the string.
//          If sending binary data, set ibuflen to the size of the binary data block.
// Return value: true - success; false - failure, indicating that the socket connection is no longer available.
bool TcpWrite(const int sockfd, const char *buffer, const int ibuflen)
{
  if (sockfd == -1) return false;

  int ilen = 0; // Message length.

  // If ibuflen == 0, it means we are sending a string, and the message length is the length of the string.
  if (ibuflen == 0)
    ilen = strlen(buffer);
  else
    ilen = ibuflen;

  int ilenn = htonl(ilen); // Convert the message length to network byte order.

  char TBuffer[ilen + 4]; // Sending buffer.
  memset(TBuffer, 0, sizeof(TBuffer)); // Clear the sending buffer.
  memcpy(TBuffer, &ilenn, 4);         // Copy the message length to the buffer.
  memcpy(TBuffer + 4, buffer, ilen);  // Copy the message content to the buffer.

  // Send the data in the sending buffer.
  if (Writen(sockfd, TBuffer, ilen + 4) == false) return false;

  return true;
}

// Read data from a socket that is ready for reading.
// sockfd: The socket connection that is ready.
// buffer: The address of the data reception buffer.
// n: The number of bytes to be received in this call.
// Return value: true - successfully received n bytes of data; false - the socket connection is no longer available.
bool Readn(const int sockfd, char *buffer, const size_t n)
{
  int nLeft = n; // Remaining number of bytes to read.
  int idx = 0;   // Number of bytes successfully read.
  int nread;     // Number of bytes read in each call to recv().

  while (nLeft > 0)
  {
    if ((nread = recv(sockfd, buffer + idx, nLeft, 0)) <= 0) return false;

    idx = idx + nread;
    nLeft = nLeft - nread;
  }

  return true;
}

// Write data to a socket that is ready for writing.
// sockfd: The socket connection that is ready.
// buffer: The address of the data to be sent.
// n: The number of bytes to be sent.
// Return value: true - successfully sent n bytes of data; false - the socket connection is no longer available.
bool Writen(const int sockfd, const char *buffer, const size_t n)
{
  int nLeft = n; // Remaining number of bytes to write.
  int idx = 0;   // Number of bytes successfully written.
  int nwritten;  // Number of bytes written in each call to send().

  while (nLeft > 0)
  {
    if ((nwritten = send(sockfd, buffer + idx, nLeft, 0)) <= 0) return false;

    nLeft = nLeft - nwritten;
    idx = idx + nwritten;
  }

  return true;
}

// Copy a file, similar to the Linux "cp" command.
// srcfilename: The name of the source file, it is recommended to use the absolute path of the file.
// dstfilename: The name of the destination file, it is recommended to use the absolute path of the file.
// Return value: true - copy succeeded; false - copy failed, mainly due to insufficient permissions or insufficient disk space.
// Note:
// 1) Before copying the file, the destination directory specified in the dstfilename parameter will be created automatically.
// 2) During the copying process, a temporary file naming method is used, and the file is renamed to dstfilename after copying is completed to avoid reading the intermediate state file.
// 3) The modified time of the copied file remains the same as the original file, which is different from the Linux "cp" command.
bool COPY(const char *srcfilename, const char *dstfilename)
{
  if (MKDIR(dstfilename) == false) return false;

  char strdstfilenametmp[301];
  SNPRINTF(strdstfilenametmp, sizeof(strdstfilenametmp), 300, "%s.tmp", dstfilename);

  int srcfd, dstfd;

  srcfd = dstfd = -1;

  int iFileSize = FileSize(srcfilename);

  int bytes = 0;
  int total_bytes = 0;
  int onread = 0;
  char buffer[5000];

  if ((srcfd = open(srcfilename, O_RDONLY)) < 0) return false;

  if ((dstfd = open(strdstfilenametmp, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IXUSR)) < 0)
  {
    close(srcfd);
    return false;
  }

  while (true)
  {
    memset(buffer, 0, sizeof(buffer));

    if ((iFileSize - total_bytes) > 5000)
      onread = 5000;
    else
      onread = iFileSize - total_bytes;

    bytes = read(srcfd, buffer, onread);

    if (bytes > 0)
      write(dstfd, buffer, bytes);

    total_bytes = total_bytes + bytes;

    if (total_bytes == iFileSize) break;
  }

  close(srcfd);
  close(dstfd);

  // Change the modification time attribute of the file
  char strmtime[21];
  memset(strmtime, 0, sizeof(strmtime));
  FileMTime(srcfilename, strmtime);
  UTime(strdstfilenametmp, strmtime);

  if (RENAME(strdstfilenametmp, dstfilename) == false)
  {
    REMOVE(strdstfilenametmp);
    return false;
  }

  return true;
}



// CTimer Constructor
CTimer::CTimer()
{
  memset(&m_start, 0, sizeof(struct timeval));
  memset(&m_end, 0, sizeof(struct timeval));

  // Start timing
  Start();
}

// Start timing
void CTimer::Start()
{
  gettimeofday(&m_start, 0);
}

// Calculate the elapsed time in seconds, with microseconds as decimals.
// After each call to this method, Start method is automatically called to restart timing.
double CTimer::Elapsed()
{
  gettimeofday(&m_end, 0);

  double dstart, dend;
  dstart = dend = 0;

  char strtemp[51];
  SNPRINTF(strtemp, sizeof(strtemp), 30, "%ld.%06ld", m_start.tv_sec, m_start.tv_usec);
  dstart = atof(strtemp);

  SNPRINTF(strtemp, sizeof(strtemp), 30, "%ld.%06ld", m_end.tv_sec, m_end.tv_usec);
  dend = atof(strtemp);

  // Restart timing
  Start();

  return dend - dstart;
}

// CSEM Constructor
CSEM::CSEM()
{
  m_semid = -1;
  m_sem_flg = SEM_UNDO;
}

// If the semaphore exists, get the semaphore; if the semaphore does not exist, create it and initialize it with the given value.
bool CSEM::init(key_t key, unsigned short value, short sem_flg)
{
  if (m_semid != -1)
    return false;

  m_sem_flg = sem_flg;

  // Semaphore initialization cannot be directly done with semget(key, 1, 0666 | IPC_CREAT) because the initial value of the semaphore will be 0 after creation.

  // The semaphore initialization is done in three steps:
  // 1) Get the semaphore, if successful, the function returns.
  // 2) If failed, create the semaphore.
  // 3) Set the initial value of the semaphore.

  // Get the semaphore.
  if ((m_semid = semget(key, 1, 0666)) == -1)
  {
    // If the semaphore does not exist, create it.
    if (errno == 2)
    {
      // Use IPC_EXCL flag to ensure that only one process creates and initializes the semaphore, while other processes can only get it.
      if ((m_semid = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
      {
        if (errno != EEXIST)
        {
          perror("init 1 semget()");
          return false;
        }
        if ((m_semid = semget(key, 1, 0666)) == -1)
        {
          perror("init 2 semget()");
          return false;
        }

        return true;
      }

      // After successful creation of the semaphore, it needs to be initialized with the given value.
      union semun sem_union;
      sem_union.val = value; // Set the initial value of the semaphore.
      if (semctl(m_semid, 0, SETVAL, sem_union) < 0)
      {
        perror("init semctl()");
        return false;
      }
    }
    else
    {
      perror("init 3 semget()");
      return false;
    }
  }

  return true;
}


// P operation on the semaphore.
bool CSEM::P(short sem_op)
{
  if (m_semid == -1)
    return false;

  struct sembuf sem_b;
  sem_b.sem_num = 0;     // Semaphore number, 0 represents the first semaphore.
  sem_b.sem_op = sem_op; // P operation requires sem_op to be less than 0.
  sem_b.sem_flg = m_sem_flg;
  if (semop(m_semid, &sem_b, 1) == -1)
  {
    perror("p semop()");
    return false;
  }

  return true;
}

// V operation on the semaphore.
bool CSEM::V(short sem_op)
{
  if (m_semid == -1)
    return false;

  struct sembuf sem_b;
  sem_b.sem_num = 0;     // Semaphore number, 0 represents the first semaphore.
  sem_b.sem_op = sem_op; // V operation requires sem_op to be greater than 0.
  sem_b.sem_flg = m_sem_flg;
  if (semop(m_semid, &sem_b, 1) == -1)
  {
    perror("V semop()");
    return false;
  }

  return true;
}

// Get the value of the semaphore. Returns the value on success, -1 on failure.
int CSEM::value()
{
  return semctl(m_semid, 0, GETVAL);
}

// Destroy the semaphore.
bool CSEM::destroy()
{
  if (m_semid == -1)
    return false;

  if (semctl(m_semid, 0, IPC_RMID) == -1)
  {
    perror("destroy semctl()");
    return false;
  }

  return true;
}

CSEM::~CSEM()
{
}

// CPActive Constructor
CPActive::CPActive()
{
  m_shmid = 0;
  m_pos = -1;
  m_shm = 0;
}

// Add the heartbeat information of the current process to the shared memory process group.
bool CPActive::AddPInfo(const int timeout, const char *pname, CLogFile *logfile)
{
  if (m_pos != -1)
    return true;

  if (m_sem.init(SEMKEYP) == false) // Initialize the semaphore.
  {
    if (logfile != 0)
      logfile->Write("Failed to create/get semaphore (%x).\n", SEMKEYP);
    else
      printf("Failed to create/get semaphore (%x).\n", SEMKEYP);

    return false;
  }

  // Create/get shared memory with key SHMKEYP and size of MAXNUMP st_procinfo structures.
  if ((m_shmid = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(struct st_procinfo), 0666 | IPC_CREAT)) == -1)
  {
    if (logfile != 0)
      logfile->Write("Failed to create/get shared memory (%x).\n", SHMKEYP);
    else
      printf("Failed to create/get shared memory (%x).\n", SHMKEYP);

    return false;
  }

  // Attach shared memory to the current process's address space.
  m_shm = (struct st_procinfo *)shmat(m_shmid, 0, 0);

  struct st_procinfo stprocinfo; // Structure for the heartbeat information of the current process.
  memset(&stprocinfo, 0, sizeof(stprocinfo));

  stprocinfo.pid = getpid();               // Current process ID.
  stprocinfo.timeout = timeout;            // Timeout value.
  stprocinfo.atime = time(0);              // Current time.
  STRNCPY(stprocinfo.pname, sizeof(stprocinfo.pname), pname, 50); // Process name.

  // Process ID is reused, and if a process exits without cleaning its heartbeat information,
  // its information will remain in shared memory. If the current process reuses that ID,
  // there will be two records with the same process ID in the shared memory. When the daemon
  // process checks the remaining process's heartbeat, it may mistakenly send a termination signal
  // to the current process.

  // If the current process number exists in shared memory, it must be data left by other processes,
  // and the current process will reuse that position.
  for (int ii = 0; ii < MAXNUMP; ii++)
  {
    if ((m_shm + ii)->pid == stprocinfo.pid)
    {
      m_pos = ii;
      break;
    }
  }

  m_sem.P(); // Lock the shared memory.

  if (m_pos == -1)
  {
    // If m_pos == -1, the process number of the current process does not exist in the shared memory process group,
    // so find an empty position.
    for (int ii = 0; ii < MAXNUMP; ii++)
    {
      if ((m_shm + ii)->pid == 0)
      {
        m_pos = ii;
        break;
      }
    }
  }

  if (m_pos == -1)
  {
    if (logfile != 0)
      logfile->Write("Shared memory space is full.\n");
    else
      printf("Shared memory space is full.\n");

    m_sem.V(); // Unlock.

    return false;
  }

  // Store the heartbeat information of the current process into the shared memory process group.
  memcpy(m_shm + m_pos, &stprocinfo, sizeof(struct st_procinfo));

  m_sem.V(); // Unlock.

  return true;
}

// Update the heartbeat time of the current process in the shared memory process group.
bool CPActive::UptATime()
{
  if (m_pos == -1)
    return false;

  (m_shm + m_pos)->atime = time(0);

  return true;
}

CPActive::~CPActive()
{
  // Remove the current process from the shared memory process group.
  if (m_pos != -1)
    memset(m_shm + m_pos, 0, sizeof(struct st_procinfo));

  // Detach the shared memory from the current process.
  if (m_shm != 0)
    shmdt(m_shm);
}

