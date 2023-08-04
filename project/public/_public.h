/****************************************************************************************/
/* Program Name: _public.h, this program is the declaration file for common functions and classes in the development framework. */
/****************************************************************************************/

#ifndef __PUBLIC_HH
#define __PUBLIC_HH 1

#include "_cmpublic.h"

///////////////////////////////////// /////////////////////////////////////
// The following are functions and classes related to string operations

// Safe strcpy function.
// dest: Target string, no need to initialize, it will be initialized in the STRCPY function.
// destlen: Size of the memory occupied by the target string dest.
// src: Original string.
// Return value: Address of the target string dest.
// Note that the content beyond the capacity of dest will be discarded.
char *STRCPY(char* dest, const size_t destlen, const char* src);

// Safe strncpy function.
// dest: Target string, no need to initialize, it will be initialized in the STRNCPY function.
// destlen: Size of the memory occupied by the target string dest.
// src: Original string.
// n: Number of bytes to copy.
// Return value: Address of the target string dest.
// Note that the content beyond the capacity of dest will be discarded.
char *STRNCPY(char* dest, const size_t destlen, const char* src, size_t n);

// Safe strcat function.
// dest: Target string.
// destlen: Size of the memory occupied by the target string dest.
// src: String to be appended.
// Return value: Address of the target string dest.
// Note that the content beyond the capacity of dest will be discarded.
char *STRCAT(char* dest, const size_t destlen, const char* src);

// Safe strncat function.
// dest: Target string.
// destlen: Size of the memory occupied by the target string dest.
// src: String to be appended.
// n: Number of bytes to append.
// Return value: Address of the target string dest.
// Note that the content beyond the capacity of dest will be discarded.
char *STRNCAT(char* dest, const size_t destlen, const char* src, size_t n);

// Safe sprintf function.
// Output variable arguments (...) to the dest string according to the format described by fmt.
// dest: Output string, no need to initialize, it will be initialized in the SPRINTF function.
// destlen: Size of the memory occupied by the output string dest. If the length of the formatted string exceeds destlen-1, the remaining content will be discarded.
// fmt: Format control description.
// ...: Arguments to be filled into the format control description fmt.
// Return value: Length of the formatted string, which is generally not of interest to the programmer.
int SPRINTF(char *dest, const size_t destlen, const char *fmt, ...);

// Safe snprintf function.
// Output variable arguments (...) to the dest string according to the format described by fmt.
// dest: Output string, no need to initialize, it will be initialized in the SNPRINTF function.
// destlen: Size of the memory occupied by the output string dest. If the length of the formatted string exceeds destlen-1, the remaining content will be discarded.
// n: Truncate the formatted string to n-1 and store it in dest. If n>destlen-1, destlen-1 will be taken.
// fmt: Format control description.
// ...: Arguments to be filled into the format control description fmt.
// Return value: Length of the formatted string, which is generally not of interest to the programmer.
// Note: The usage of the third argument n in the snprintf function is slightly different on Windows and Linux platforms. Assume that the length of the formatted string exceeds 10, if the third argument n is set to 10, then on Windows, the length of dest will be 10, while on Linux, the length of dest will be 9.
int SNPRINTF(char *dest, const size_t destlen, size_t n, const char *fmt, ...);

// Delete the specified character from the left of the string.
// str: The string to be processed.
// chr: The character to be deleted.
void DeleteLChar(char *str, const char chr);

// Delete the specified character from the right of the string.
// str: The string to be processed.
// chr: The character to be deleted.
void DeleteRChar(char *str, const char chr);

// Delete the specified character from both the left and right sides of the string.
// str: The string to be processed.
// chr: The character to be deleted.
void DeleteLRChar(char *str, const char chr);

// Convert lowercase letters in the string to uppercase, ignoring non-letter characters.
// str: The string to be converted, supports both char[] and string types.
void ToUpper(char *str);
void ToUpper(string &str);

// Convert uppercase letters in the string to lowercase, ignoring non-letter characters.
// str: The string to be converted, supports both char[] and string types.
void ToLower(char *str);
void ToLower(string &str);

// String replacement function.
// In the string str, if there is the string str1, it will be replaced with the string str2.
// str: The string to be processed.
// str1: Old content.
// str2: New content.
// bloop: Whether to perform replacement in a loop.
// Note:
// 1. If str2 is longer than str1, the length of str will be increased after replacement, so it is necessary to ensure that str has enough space, otherwise memory overflow may occur.
// 2. If str2 contains the content of str1 and bloop is true, this approach has logical errors, and UpdateStr will do nothing.
void UpdateStr(char *str, const char *str1, const char *str2, const bool bloop = true);

// Extract numbers, symbols, and decimals from one string and store them in another string.
// src: The original string.
// dest: The target string.
// bsigned: Whether to include symbols (+ and -), true - include; false - do not include.
// bdot: Whether to include the decimal point symbol, true - include; false - do not include.
void PickNumber(const char *src, char *dest, const bool bsigned, const bool bdot);

// Regular expression, to judge if one string matches another string.
// str: The string to be judged, it is represented accurately, such as the file name "_public.cpp".
// rules: Expression for matching rules, using an asterisk "*" to represent any string, multiple expressions are separated by commas, such as "*.h,*.cpp".
// Note: 1) The str parameter does not support "*", but the rules parameter supports "*"; 2) The function ignores the case when judging whether str matches rules.
bool MatchStr(const string &str, const string &rules);
///////////////////////////////////// /////////////////////////////////////


// The CCmdStr class is used to split a string with a delimiter.
// The format of the string is: field content 1 + delimiter + field content 2 + delimiter + field content 3 + ... + field content n.
// For example: "messi,10,striker,30,1.72,68.5,Barcelona" represents the information of a football player Messi, including name, jersey number, position on the field, age, height, weight, and club, separated by commas.
class CCmdStr
{
public:
  vector<string> m_vCmdStr; // Stores the split field contents.

  CCmdStr(); // Constructor.
  CCmdStr(const string &buffer, const char *sepstr, const bool bdelspace = false);

  // Split the string into the m_vCmdStr container.
  // buffer: The string to be split.
  // sepstr: The delimiter used in buffer. Note that the data type of the sepstr parameter is not a character, but a string, such as ",", " ", "|", "~!~".
  // bdelspace: Whether to delete the spaces before and after the field content after splitting, true - delete; false - do not delete, default is not deleted.
  void SplitToCmd(const string &buffer, const char *sepstr, const bool bdelspace = false);

  // Get the number of split fields, i.e., the size of the m_vCmdStr container.
  int CmdCount();

  // Get the field content from the m_vCmdStr container.
  // inum: The sequence number of the field, similar to the index of an array, starting from 0.
  // value: The address of the variable to store the field content.
  // Return value: true - success; if the value of inum exceeds the size of the m_vCmdStr container, it returns false.
  bool GetValue(const int inum, char *value, const int ilen = 0); // String, ilen default value is 0.
  bool GetValue(const int inum, int  *value); // Integer.
  bool GetValue(const int inum, unsigned int *value); // Unsigned integer.
  bool GetValue(const int inum, long *value); // Long integer.
  bool GetValue(const int inum, unsigned long *value); // Unsigned long integer.
  bool GetValue(const int inum, double *value); // Double precision double.
  bool GetValue(const int inum, bool *value); // Boolean.

  ~CCmdStr(); // Destructor.
};
///////////////////////////////////// /////////////////////////////////////


///////////////////////////////////// /////////////////////////////////////
// A family of functions to parse XML-formatted strings.
// The content of the XML-formatted string is as follows:
// <filename>/tmp/_public.h</filename><mtime>2020-01-01 12:20:35</mtime><size>18348</size>
// <filename>/tmp/_public.cpp</filename><mtime>2020-01-01 10:10:15</mtime><size>50945</size>
// xmlbuffer: The XML-formatted string to be parsed.
// fieldname: The tag name of the field.
// value: The address of the variable to store the field content, supports bool, int, unsigned int, long, unsigned long, double, and char[].
// Note that when the data type of the value parameter is char[], it must ensure that the memory of value array is sufficient, otherwise, memory overflow may occur. The ilen parameter can also be used to limit the length of the field content to be obtained, and the default value of ilen is 0, which means unlimited length.
// Return value: true - success; if the tag name specified by the fieldname parameter does not exist, it returns false.
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, char *value, const int ilen = 0);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, bool *value);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, int  *value);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, unsigned int *value);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, long *value);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, unsigned long *value);
bool GetXMLBuffer(const char *xmlbuffer, const char *fieldname, double *value);
///////////////////////////////////// /////////////////////////////////////

///////////////////////////////////// /////////////////////////////////////
/*
  Get the time of the operating system.
  stime: Used to store the obtained time string.
  timetvl: Time offset, unit: seconds, 0 is the default value, which means the current time; 30 means 30 seconds after the current time, -30 means 30 seconds before the current time.
  fmt: The format of the output time string stime, the same as the fmt parameter in the LocalTime function. If the format of fmt is incorrect, stime will be empty.
*/
void LocalTime(char *stime, const char *fmt = 0, const int timetvl = 0);

// Convert time represented as an integer to time represented as a string.
// ltime: Time represented as an integer.
// stime: Time represented as a string.
// fmt: The format of the output string time stime, which is the same as the fmt parameter in the LocalTime function. If the format of fmt is incorrect, stime will be empty.
void timetostr(const time_t ltime, char *stime, const char *fmt = 0);

// Convert time represented as a string to time represented as an integer.
// stime: Time represented as a string, the format is not limited, but yyyymmddhh24miss must be included, one cannot be missing, and the order cannot be changed.
// Return value: Time represented as an integer, if the format of stime is incorrect, it returns -1.
time_t strtotime(const char *stime);

// Add a number of seconds to the string representation of time to get a new string representation of time.
// in_stime: Input string format time, the format is not limited, but yyyymmddhh24miss must be included, one cannot be missing, and the order cannot be changed.
// out_stime: Output string format time.
// timetvl: Number of seconds to be offset, positive value means offset backward, negative value means offset forward.
// fmt: The format of the output string time out_stime, the same as the fmt parameter in the LocalTime function.
// Note: The in_stime and out_stime parameters can be the addresses of the same variable. If the call fails, the content of out_stime will be cleared.
// Return value: true - success, false - failure, if it returns failure, it can be considered that the format of in_stime is incorrect.
bool AddTime(const char *in_stime, char *out_stime, const int timetvl, const char *fmt = 0);
///////////////////////////////////// /////////////////////////////////////

///////////////////////////////////// /////////////////////////////////////
// This is a timer accurate to microseconds.
class CTimer
{
private:
public:
  struct timeval m_start; // The time when timing starts.
  struct timeval m_end; // The time when timing is completed.

  // Start timing.
  void Start();
  CTimer(); // The Start method is called automatically in the constructor.

  // Calculate the elapsed time, in seconds, with the decimal point followed by microseconds.
  // After each call to this method, the Start method is automatically called to restart the timing.
  double Elapsed();
};


///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////// /////////////////////////////////////
// Classes related to directory operations.

// Create directories recursively based on the absolute path of the file name or directory name.
// pathorfilename: Absolute path of the file name or directory name.
// bisfilename: Specifies the type of pathorfilename. true - pathorfilename is a file name, otherwise, it's a directory name. Default value is true.
// Returns: true - success, false - failure. If the function returns false, it could be due to three reasons: 1) insufficient permissions; 2) invalid pathorfilename; 3) insufficient disk space.
bool MKDIR(const char *pathorfilename, bool bisfilename = true);

// Get the file list information in a directory and its subdirectories.
class CDir
{
public:
  char m_DirName[301];        // Directory name, e.g., /tmp/root.
  char m_FileName[301];       // File name, excluding the directory name, e.g., data.xml.
  char m_FullFileName[301];   // Full file name, including the directory name, e.g., /tmp/root/data.xml.
  int  m_FileSize;            // File size in bytes.
  char m_ModifyTime[21];      // Last modified time of the file, represented as st_mtime member of the stat structure.
  char m_CreateTime[21];      // File creation time, represented as st_ctime member of the stat structure.
  char m_AccessTime[21];      // Last access time of the file, represented as st_atime member of the stat structure.
  char m_DateFMT[25];         // File time display format, set by the SetDateFMT method.

  vector<string> m_vFileName; // Stores the absolute path file name list obtained from the OpenDir method.
  int m_pos;                  // Current position of reading m_vFileName container. The m_pos is increased by 1 for each call of the ReadDir method.

  CDir();  // Constructor.

  void initdata(); // Initialize member variables.

  // Set the file time format, supports two formats: "yyyy-mm-dd hh24:mi:ss" and "yyyymmddhh24miss". The default is the former.
  void SetDateFMT(const char *in_DateFMT);

  // Open a directory and get the file list information, store it in the m_vFileName container.
  // in_DirName: The directory to be opened, in absolute path format, e.g., /tmp/root.
  // in_MatchStr: The file name matching rule to obtain file names. Files that do not match will be ignored. See the MatchStr function in the development framework for details.
  // in_MaxCount: The maximum number of files to be obtained. Default value is 10000.
  // bAndChild: Whether to open subdirectories. Default value is false - do not open subdirectories.
  // bSort: Whether to sort the obtained file list (i.e., the contents of the m_vFileName container). Default value is false - do not sort.
  // Returns: true - success, false - failure. If the in_DirName specified directory does not exist, the OpenDir method will create it. If the creation fails, it returns false. If the current user does not have read permission for subdirectories under in_DirName, it also returns false.
  bool OpenDir(const char *in_DirName, const char *in_MatchStr, const unsigned int in_MaxCount = 10000, const bool bAndChild = false, bool bSort = false);

  // This is a recursive function called by OpenDir(). It doesn't need to be called externally from the CDir class.
  bool _OpenDir(const char *in_DirName, const char *in_MatchStr, const unsigned int in_MaxCount, const bool bAndChild);

  // Get one record (file name) from the m_vFileName container and get the file size, modification time, and other information of the file.
  // When calling the OpenDir method, the m_vFileName container is cleared, and m_pos is reset to zero. Each call to ReadDir method increases m_pos by 1.
  // Returns: true - success, false - failure. If m_pos is less than m_vFileName.size(), it returns true; otherwise, it returns false.
  bool ReadDir();

  ~CDir();  // Destructor.
};


///////////////////////////////////// /////////////////////////////////////

///////////////////////////////////// /////////////////////////////////////
// Functions and classes related to file operations.

// Delete a file, similar to the Linux system's rm command.
// filename: The file name to be deleted. It is recommended to use the absolute path of the file, e.g., /tmp/root/data.xml.
// times: The number of times to attempt file deletion. Default is 1. It is recommended not to exceed 3. From practical experience, if the first deletion attempt fails, retrying 2 more times is usually sufficient. If deletion fails, usleep(100000) and retry.
// Returns: true - success, false - failure. The main reason for failure is insufficient permissions.
// In application development, you can use the REMOVE function instead of the remove library function.
bool REMOVE(const char *filename, const int times = 1);

// Rename a file, similar to the Linux system's mv command.
// srcfilename: The original file name, recommended to use the absolute path of the file.
// dstfilename: The target file name, recommended to use the absolute path of the file.
// times: The number of times to attempt file renaming. Default is 1. It is recommended not to exceed 3. From practical experience, if the first renaming attempt fails, retrying 2 more times is usually sufficient. If renaming fails, usleep(100000) and retry.
// Returns: true - success, false - failure. The main reasons for failure are insufficient permissions or insufficient disk space. If the source file and target file are not on the same disk partition, renaming may also fail.
// Note: Before renaming the file, the directories in the dstfilename parameter will be automatically created.
// In application development, you can use the RENAME function instead of the rename library function.
bool RENAME(const char *srcfilename, const char *dstfilename, const int times = 1);

// Copy a file, similar to the Linux system's cp command.
// srcfilename: The original file name, recommended to use the absolute path of the file.
// dstfilename: The target file name, recommended to use the absolute path of the file.
// Returns: true - success, false - failure. The main reasons for failure are insufficient permissions or insufficient disk space.
// Note:
// 1) Before copying the file, the directory in the dstfilename parameter will be automatically created.
// 2) The copying process uses a temporary file-naming method. After copying is complete, the file is renamed to dstfilename to avoid the intermediate state file being read.
// 3) The time of the copied file remains the same as the original file, which is different from the Linux system's cp command.
bool COPY(const char *srcfilename, const char *dstfilename);

// Get the size of a file.
// filename: The file name to get the size, recommended to use the absolute path of the file.
// Returns: If the file does not exist or there is no access permission, returns -1. If successful, returns the size of the file in bytes.
int FileSize(const char *filename);

// Get the time of a file.
// filename: The file name to get the time, recommended to use the absolute path of the file.
// mtime: Used to store the time of the file, i.e., st_mtime member of the stat structure.
// fmt: Set the output format of the time, same as the LocalTime function, but the default is "yyyymmddhh24miss".
// Returns: If the file does not exist or there is no access permission, returns false. If successful, returns true.
bool FileMTime(const char *filename, char *mtime, const char *fmt = 0);

// Reset the modification time attribute of a file.
// filename: The file name to reset the modification time, recommended to use the absolute path of the file.
// stime: The time represented as a string. It must contain yyyymmddhh24miss, and the order cannot be changed.
// Returns: true - success, false - failure. The reason for failure is saved in errno.
bool UTime(const char *filename, const char *mtime);

// Open a file.
// The FOPEN function calls the fopen library function to open a file. If the directory in the file name does not exist, it will be created.
// The parameters and return value of the FOPEN function are exactly the same as the fopen function.
// In application development, you can use the FOPEN function instead of the fopen library function.
FILE *FOPEN(const char *filename, const char *mode);

// Read a line from a text file.
// fp: The already opened file pointer.
// buffer: Used to store the read content. buffer must be larger than readsize+1, otherwise, it may result in incomplete data read or memory overflow.
// readsize: The number of bytes to read in this call. If the endbz (line content ending flag) is encountered, the function returns.
// endbz: The flag for the end of line content. Default is empty, which means the line content ends with "\n".
// Returns: true - success, false - failure. In general, failure can be considered as the end of the file.
bool FGETS(const FILE *fp, char *buffer, const int readsize, const char *endbz = 0);


// File operation class declaration
class CFile
{
private:
  FILE *m_fp;        // File pointer
  bool  m_bEnBuffer; // Enable buffer flag, true - enable; false - disable (default is enabled).
  char  m_filename[301]; // File name, it is recommended to use an absolute path.
  char  m_filenametmp[301]; // Temporary file name, adding ".tmp" after m_filename.

public:
  CFile();   // Constructor.

  bool IsOpened();  // Check if the file is opened. Returns true if the file is opened; otherwise, returns false.

  // Open the file.
  // filename: File name to open, it is recommended to use an absolute path.
  // openmode: Open mode for the file, same as the mode in the fopen library function.
  // bEnBuffer: Enable buffer flag, true - enable; false - disable (default is enabled).
  // Note: If the directory for the file does not exist, it will be created.
  bool Open(const char *filename, const char *openmode, bool bEnBuffer = true);

  // Close the file pointer and delete the file.
  bool CloseAndRemove();

  // Open the file for renaming, parameters are the same as Open method.
  // Note: OpenForRename opens the temporary file with ".tmp" appended to filename, so openmode can only be "a", "a+", "w", or "w+".
  bool OpenForRename(const char *filename, const char *openmode, bool bEnBuffer = true);

  // Close the file pointer and rename the temporary file created by the OpenForRename method to filename.
  bool CloseAndRename();

  // Use fprintf to write data to the file. Parameters are the same as the fprintf library function, but without passing the file pointer.
  void Fprintf(const char *fmt, ...);

  // Read a line from the file that ends with a newline character "\n", similar to fgets function.
  // buffer: Buffer to store the read content. The buffer must be larger than readsize+1, otherwise it may cause memory overflow.
  // readsize: The number of bytes intended to read this time. If the end flag "\n" has been read, the function returns.
  // bdelcrt: Whether to remove the line ending flags "\r" and "\n", true - remove; false - do not remove (default is false).
  // Returns: true - success; false - failure. In general, failure can be considered as the end of the file.
  bool Fgets(char *buffer, const int readsize, bool bdelcrt = false);

  // Read a line from the file.
  // buffer: Buffer to store the read content. The buffer must be larger than readsize+1, otherwise it may cause memory overflow.
  // readsize: The number of bytes intended to read this time. If the end flag is encountered, the function returns.
  // endbz: Ending flag of the line content. Default is empty, which means the line ends with "\n".
  // Returns: true - success; false - failure. In general, failure can be considered as the end of the file.
  bool FFGETS(char *buffer, const int readsize, const char *endbz = 0);

  // Read data blocks from the file.
  // ptr: Buffer to store the read content.
  // size: The number of bytes intended to read this time.
  // Returns: The number of bytes successfully read from the file. If the file is not ended, the return value is equal to size.
  // If the file has ended, the return value is the actual number of bytes read.
  size_t Fread(void *ptr, size_t size);

  // Write data blocks to the file.
  // ptr: Address of the data block to be written.
  // size: Number of bytes in the data block to be written.
  // Returns: The number of bytes successfully written to the file. If there is enough disk space, the return value is equal to size.
  size_t Fwrite(const void *ptr, size_t size);

  // Close the file pointer and delete the temporary file if it exists.
  void Close();

  ~CFile();   // Destructor, calls the Close method.
};


///////////////////////////////////// /////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Log file operation class
class CLogFile
{
public:
  FILE   *m_tracefp;           // Log file pointer.
  char    m_filename[301];     // Log file name, it is recommended to use an absolute path.
  char    m_openmode[11];      // Log file open mode, typically use "a+".
  bool    m_bEnBuffer;         // Whether to enable operating system buffer mechanism when writing log, default is disabled.
  bool    m_bBackup;           // Whether to automatically switch log files when the log file size exceeds m_MaxLogSize, default is enabled.
  long    m_MaxLogSize;        // Maximum size of the log file in MB, default is 100MB.
  // pthread_spinlock_t spin;  // Ignore this line for now (used for thread synchronization).

  // Constructor.
  // MaxLogSize: Maximum size of the log file in MB, default is 100MB, minimum is 10MB.
  CLogFile(const long MaxLogSize = 100);

  // Open the log file.
  // filename: Log file name, it is recommended to use an absolute path. If the directory for the file does not exist, it will be created.
  // openmode: Log file open mode, same as the mode in the fopen library function. Default value is "a+".
  // bBackup: Whether to automatically switch log files, true - switch, false - do not switch. In a multi-process service program where multiple processes share the same log file, bBackup must be false.
  // bEnBuffer: Whether to enable file buffer mechanism, true - enable, false - disable. If the buffer is enabled, the content written to the log file will not be immediately written to the file. Default is disabled.
  bool Open(const char *filename, const char *openmode = 0, bool bBackup = true, bool bEnBuffer = false);

  // If the log file is larger than m_MaxLogSize, rename the current log file to a historical log file and create a new current log file.
  // The backup file will have the date and time appended to the log file name, for example, /tmp/log/filetodb.log.20200101123025.
  // Note: In a multi-process program, log files cannot be switched; in a multi-threaded program, log files can be switched.
  bool BackupLogFile();

  // Write content to the log file. The fmt is a variable parameter, used similar to the printf library function.
  // Write method will write the current time, WriteEx method will not write time.
  bool Write(const char *fmt, ...);
  bool WriteEx(const char *fmt, ...);

  // Close the log file.
  void Close();

  ~CLogFile();  // Destructor, calls the Close method.
};

///////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * File name: hssms.xml
<?xml version="1.0" encoding="gbk" ?>
<root>
    <!-- The log file name for the program to run. -->
    <logpath>/log/hssms</logpath>

    <!-- Database connection parameters. -->
    <connstr>hssms/smspwd@hssmszx</connstr>

    <!-- The root directory for data files. -->
    <datapath>/data/hssms</datapath>

    <!-- IP address of the central server. -->
    <serverip>192.168.1.1</serverip>

    <!-- Communication port of the central server. -->
    <port>5058</port>

    <!-- Whether to use a long connection, true - yes; false - no. -->
    <online>true</online>
</root>
*/

class CIniFile
{
public:
  string m_xmlbuffer; // Stores the entire content of the parameter file loaded by the LoadFile method.

  CIniFile();

  // Load the content of the parameter file into the m_xmlbuffer member variable.
  bool LoadFile(const char *filename);
 
  // Get the value of a parameter.
  // fieldname: The tag name of the field.
  // value: A pointer to the variable where the field's value will be stored. It supports bool, int, unsigned int, long, unsigned long, double, and char[] types.
  // Note: When the data type of the value parameter is char[], make sure that the memory for value is large enough to prevent potential memory overflow. You can use the ilen parameter to limit the length of the field's content. The default value of ilen is 0, which means the content length is not limited.
  // Returns true if successful; false otherwise.
  bool GetValue(const char *fieldname, bool *value);
  bool GetValue(const char *fieldname, int *value);
  bool GetValue(const char *fieldname, unsigned int *value);
  bool GetValue(const char *fieldname, long *value);
  bool GetValue(const char *fieldname, unsigned long *value);
  bool GetValue(const char *fieldname, double *value);
  bool GetValue(const char *fieldname, char *value, const int ilen = 0);
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Socket communication client class.
class CTcpClient
{
public:
  int  m_connfd;    // Client's socket.
  char m_ip[21];    // IP address of the server.
  int  m_port;      // Port for communication with the server.
  bool m_btimeout;  // Indicates whether the Read method failed due to a timeout: true-timeout, false-no timeout.
  int  m_buflen;    // Size of the received message after calling the Read method, in bytes.

  CTcpClient();  // Constructor.

  // Initiates a connection request to the server.
  // ip: IP address of the server.
  // port: Port number the server is listening on.
  // Returns true if successful; false otherwise.
  bool ConnectToServer(const char *ip, const int port);

  // Receives data sent by the server.
  // buffer: Address of the receive data buffer, and the length of the data is stored in the m_buflen member variable.
  // itimeout: Timeout for waiting for data, in seconds. The default value is 0, which means infinite wait.
  // Returns true if successful; false otherwise. Failure can happen due to two reasons: 1) timeout, and the m_btimeout member variable will be set to true; 2) socket connection is no longer available.
  bool Read(char *buffer, const int itimeout = 0);

  // Sends data to the server.
  // buffer: Address of the data buffer to be sent.
  // ibuflen: Size of the data to be sent, in bytes. The default value is 0. If sending an ASCII string, set ibuflen to 0. For binary data, ibuflen should be the size of the binary data block.
  // Returns true if successful; false otherwise. If failed, it indicates that the socket connection is no longer available.
  bool Write(const char *buffer, const int ibuflen = 0);

  // Disconnects from the server.
  void Close();

  ~CTcpClient();  // Destructor automatically closes the socket and frees resources.
};


// Socket communication server class.
class CTcpServer
{
private:
  int m_socklen;                    // Size of struct sockaddr_in.
  struct sockaddr_in m_clientaddr;  // Client's address information.
  struct sockaddr_in m_servaddr;    // Server's address information.
public:
  int  m_listenfd;   // Socket used by the server for listening.
  int  m_connfd;     // Socket connected by the client.
  bool m_btimeout;   // Indicates whether the Read method failed due to a timeout: true-timeout, false-no timeout.
  int  m_buflen;     // Size of the received message after calling the Read method, in bytes.

  CTcpServer();  // Constructor.

  // Server initialization.
  // port: The port number the server listens on.
  // backlog: The maximum length of the pending connection queue, defaults to 5.
  // Returns true if successful; false otherwise. In general, as long as the port is set correctly and not occupied, initialization will succeed.
  bool InitServer(const unsigned int port, const int backlog = 5);

  // Blocking, wait for a client's connection request.
  // Returns true if a new client has connected; false otherwise. If Accept fails, it can be retried.
  bool Accept();

  // Get the client's IP address.
  // Returns the client's IP address as a string, e.g., "192.168.1.100".
  char* GetIP();

  // Receives data sent by the client.
  // buffer: Address of the receive data buffer, and the length of the data is stored in the m_buflen member variable.
  // itimeout: Timeout for waiting for data, in seconds. The default value is 0, which means infinite wait.
  // Returns true if successful; false otherwise. Failure can happen due to two reasons: 1) timeout, and the m_btimeout member variable will be set to true; 2) socket connection is no longer available.
  bool Read(char* buffer, const int itimeout = 0);

  // Sends data to the client.
  // buffer: Address of the data buffer to be sent.
  // ibuflen: Size of the data to be sent, in bytes. The default value is 0. If sending an ASCII string, set ibuflen to 0. For binary data, ibuflen should be the size of the binary data block.
  // Returns true if successful; false otherwise. If failed, it indicates that the socket connection is no longer available.
  bool Write(const char* buffer, const int ibuflen = 0);

  // Close the listening socket, m_listenfd, commonly used in the child process code of multi-process service programs.
  void CloseListen();

  // Close the client's socket, m_connfd, commonly used in the parent process code of multi-process service programs.
  void CloseClient();

  ~CTcpServer();  // Destructor automatically closes the socket and frees resources.
};


// Receive data sent by the other end of the socket.
// sockfd: The valid socket connection.
// buffer: Address of the receive data buffer.
// ibuflen: Number of bytes successfully received in this operation.
// itimeout: Timeout for receiving data, in seconds. -1-No waiting; 0-Infinite wait; >0-Number of seconds to wait.
// Returns true if successful; false otherwise. Failure can happen due to two reasons: 1) timeout; 2) socket connection is no longer available.
bool TcpRead(const int sockfd, char* buffer, int* ibuflen, const int itimeout = 0);

// Send data to the other end of the socket.
// sockfd: The valid socket connection.
// buffer: Address of the data buffer to be sent.
// ibuflen: Size of the data to be sent, in bytes. If sending an ASCII string, set ibuflen to 0 or the length of the string; for binary data, ibuflen should be the size of the binary data block.
// Returns true if successful; false otherwise. If failed, it indicates that the socket connection is no longer available.
bool TcpWrite(const int sockfd, const char* buffer, const int ibuflen = 0);

// Read data from a socket that is ready for reading.
// sockfd: The socket connection that is ready for reading.
// buffer: Address of the receive data buffer.
// n: Number of bytes to receive in this operation.
// Returns true after successfully receiving n bytes of data; false if the socket connection is no longer available.
bool Readn(const int sockfd, char* buffer, const size_t n);

// Write data to a socket that is ready for writing.
// sockfd: The socket connection that is ready for writing.
// buffer: Address of the data buffer to be sent.
// n: Number of bytes to send.
// Returns true after successfully sending n bytes of data; false if the socket connection is no longer available.
bool Writen(const int sockfd, const char* buffer, const size_t n);

// The above are functions and classes for socket communication.
///////////////////////////////////// /////////////////////////////////////

// Close all signals and IO. By default, only signals are closed, and IO is not closed.
void CloseIOAndSignal(bool bCloseIO = false);


// Semaphore.
class CSEM
{
private:
  union semun // Union used for semaphore operations.
  {
    int val;
    struct semid_ds* buf;
    unsigned short* arry;
  };

  int m_semid; // Semaphore descriptor.

  // If sem_flg is set to SEM_UNDO, the operating system will track the modifications to the semaphore by processes.
  // After all processes (normal or abnormal) that modified the semaphore have terminated, the operating system will restore the semaphore to its initial value (as if all processes' operations on the semaphore were undone).
  // If the semaphore is used to represent the quantity of available resources (unchanging), setting SEM_UNDO is more appropriate.
  // If the semaphore is used in a producer-consumer model, setting it to 0 is more appropriate.
  // Note that most information found online about sem_flg is incorrect. It is important to test it thoroughly by yourself.
  short m_sem_flg;

public:
  CSEM();
  // If the semaphore already exists, get the semaphore; if it does not exist, create it and initialize it to value.
  bool init(key_t key, unsigned short value = 1, short sem_flg = SEM_UNDO);
  bool P(short sem_op = -1); // Semaphore P operation.
  bool V(short sem_op = 1);  // Semaphore V operation.
  int value();               // Get the value of the semaphore. Returns the semaphore's value if successful, -1 if failed.
  bool destroy();            // Destroy the semaphore.
  ~CSEM();
};

// Process heartbeat information structure.
struct st_procinfo
{
  int pid;         // Process ID.
  char pname[51];  // Process name, can be empty.
  int timeout;     // Timeout in seconds.
  time_t atime;    // Time of the last heartbeat, represented as an integer.
};


#define MAXNUMP     1000    // Maximum number of processes.
#define SHMKEYP   0x5095    // Shared memory key.
#define SEMKEYP   0x5095    // Semaphore key.

// View shared memory: ipcs -m
// Delete shared memory: ipcrm -m shmid
// View semaphores: ipcs -s
// Delete semaphores: ipcrm sem semid

// Process heartbeat operation class.
class CPActive
{
private:
  CSEM m_sem;                 // Semaphore ID used to lock shared memory.
  int  m_shmid;               // Shared memory ID.
  int  m_pos;                 // Current process position in the shared memory process group.
  st_procinfo* m_shm;         // Pointer to the shared memory address space.

public:
  CPActive();  // Initialize member variables.

  // Add the current process's heartbeat information to the shared memory process group.
  bool AddPInfo(const int timeout, const char* pname = 0, CLogFile* logfile = 0);

  // Update the heartbeat time of the current process in the shared memory process group.
  bool UptATime();

  ~CPActive();  // Remove the current process's heartbeat record from shared memory.
};

#endif

