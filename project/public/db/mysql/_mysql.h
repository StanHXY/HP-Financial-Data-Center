/**************************************************************************************/
/*   Program: _mysql.h, this program is the declaration file for C/C++ operations with MySQL database in a development framework. */
/*   Author: Wu Congzhou.                                                                  */
/**************************************************************************************/

#ifndef __MYSQL_H
#define __MYSQL_H

// Common C/C++ library header files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <mysql.h>   // Header file for MySQL database interface functions

// Load the file "filename" into the buffer, make sure the buffer is large enough.
// Return the size of the file on success, return 0 if the file does not exist or is empty.
unsigned long filetobuf(const char *filename, char *buffer);

// Write the content of the buffer to the file "filename", with "size" being the size of the valid content in the buffer.
// Return true on success, false on failure.
bool buftofile(const char *filename, char *buffer, unsigned long size);

// MySQL login environment
struct LOGINENV
{
  char ip[32];       // MySQL database IP address.
  int  port;         // Communication port of MySQL database.
  char user[32];     // Username to login to MySQL database.
  char pass[32];     // Password to login to MySQL database.
  char dbname[51];   // Default database to open after login.
};

struct CDA_DEF         // Result returned each time a MySQL interface function is called.
{
  int      rc;         // Return value: 0-Success; Others-Failure, contains MySQL error code.
  unsigned long rpc;   // If it is insert, update, or delete, it holds the number of affected rows; if it is a select, it holds the number of rows in the result set.
  char     message[2048]; // If there is a failure, it holds the error description.
};

// MySQL database connection class.
class connection
{
private:
  // Parse ip, username, password, dbname, and port from connstr.
  void setdbopt(const char *connstr);

  // Set character set, it must be consistent with the database to avoid Chinese garbled characters.
  void character(const char *charset);

  LOGINENV m_env;      // Server environment handle.

  char m_dbtype[21];   // Type of the database, fixed value is "mysql".
public:
  int m_state;         // Connection status with the database, 0-Not connected, 1-Connected.

  CDA_DEF m_cda;       // Result of database operations or the last execution of SQL statement.

  char m_sql[10241];   // Text of the SQL statement, maximum length cannot exceed 10240 bytes.

  connection();        // Constructor.
  ~connection();        // Destructor.

  // Log in to the database.
  // connstr: Database login parameters in the format: "ip,username,password,dbname,port",
  // for example: "172.16.0.15,qxidc,qxidcpwd,qxidcdb,3306".
  // charset: Character set of the database, such as "utf8", "gbk", it must be consistent with the database to avoid Chinese garbled characters.
  // autocommitopt: Whether to enable auto-commit, 0-Not enabled, 1-Enabled, default is not enabled.
  // Return value: 0-Success, Others-Failure, the failure code is in m_cda.rc, the failure description is in m_cda.message.
  int connecttodb(const char *connstr, const char *charset, unsigned int autocommitopt = 0);

  // Commit the transaction.
  // Return value: 0-Success, Others-Failure, program generally does not need to care about the return value.
  int commit();

  // Rollback the transaction.
  // Return value: 0-Success, Others-Failure, program generally does not need to care about the return value.
  int rollback();

  // Disconnect from the database.
  // Note: When disconnecting from the database, all uncommitted transactions are automatically rolled back.
  // Return value: 0-Success, Others-Failure, program generally does not need to care about the return value.
  int disconnect();

  // Execute SQL statement.
  // If the SQL statement does not require binding of input and output variables (no bound variables, non-query statement),
  // it can be directly executed using this method.
  // Parameter explanation: This is a variable-length parameter, usage is the same as the printf function.
  // Return value: 0-Success, Others-Failure, the failure code is in m_cda.rc, the failure description is in m_cda.message,
  // if a non-query statement is successfully executed, m_cda.rpc holds the number of affected rows for this SQL execution.
  // The program must check the return value of the execute method.
  // The execute method is provided in the connection class for convenience, and it uses the sqlstatement class to complete the functionality.
  int execute(const char *fmt, ...);

  ////////////////////////////////////////////////////////////////////
  // The following member variables and functions do not need to be called outside the class except for the sqlstatement class.
  MYSQL     *m_conn;   // MySQL database connection handle.
  int m_autocommitopt; // Auto-commit flag, 0-Disable auto-commit; 1-Enable auto-commit.
  void err_report();   // Get error information.
  ////////////////////////////////////////////////////////////////////
};

// The maximum number of input or output variables to bind before executing SQL statements, 256 is large enough, it can be adjusted according to the actual situation.
#define MAXPARAMS  256

// SQL statement operation class.
class sqlstatement
{
private:
  MYSQL_STMT *m_handle; // SQL statement handle.
  
  MYSQL_BIND params_in[MAXPARAMS];            // Input parameters.
  unsigned long params_in_length[MAXPARAMS];  // Actual length of input parameters.
  my_bool params_in_is_null[MAXPARAMS];       // Whether input parameters are NULL.
  unsigned maxbindin;                         // Maximum number of input parameters.

  MYSQL_BIND params_out[MAXPARAMS]; // Output parameters.

  CDA_DEF m_cda1;      // Result of prepare() SQL statement.
  
  connection *m_conn;  // Database connection pointer.
  int m_sqltype;       // Type of the SQL statement, 0-Query statement; 1-Non-query statement.
  int m_autocommitopt; // Auto-commit flag, 0-Disable; 1-Enable.
  void err_report();   // Error report.
  void initial();      // Initialize member variables.
public:
  int m_state;         // Binding status with the database, 0-Not bound, 1-Bound.

  char m_sql[10241];   // Text of the SQL statement, maximum length cannot exceed 10240 bytes.

  CDA_DEF m_cda;       // Result of executing SQL statement.

  sqlstatement();      // Constructor.
  sqlstatement(connection *conn);    // Constructor, also bind to the database connection.

  ~sqlstatement();      // Destructor.

  // Bind to the database connection.
  // conn: Address of the database connection connection object.
  // Return value: 0-Success, Others-Failure, as long as the conn parameter is valid and there are enough database cursor resources, the connect method will not return a failure.
  // The program generally does not need to care about the return value of the connect method.
  // Note: Each sqlstatement only needs to be bound once, before binding to a new connection, you must call the disconnect method first.
  int connect(connection *conn);

  // Unbind from the database connection.
  // Return value: 0-Success, Others-Failure, the program generally does not need to care about the return value.
  int disconnect();

  // Prepare the SQL statement.
  // Parameter explanation: This is a variable-length parameter, usage is the same as the printf function.
  // Return value: 0-Success, Others-Failure, the program generally does not need to care about the return value.
  // Note: If the SQL statement does not change, it only needs to be prepared once.
  int prepare(const char *fmt, ...);

  // Bind the address of the input variable.
  // position: The order of the field, starting from 1, must correspond one-to-one with the sequence number of SQL in the prepare method.
  // value: Address of the input variable, if it is a string, the memory size should be the length of the corresponding field plus 1.
  // len: If the data type of the input variable is a string, use len to specify its maximum length, it is recommended to use the length of the corresponding field.
  // Return value: 0-Success, Others-Failure, the program generally does not need to care about the return value.
  // Note: 1) If the SQL statement does not change, it only needs to bind once; 2) The total number of bound input variables cannot exceed MAXPARAMS.
  int bindin(unsigned int position, int    *value);
  int bindin(unsigned int position, long   *value);
  int bindin(unsigned int position, unsigned int  *value);
  int bindin(unsigned int position, unsigned long *value);
  int bindin(unsigned int position, float *value);
  int bindin(unsigned int position, double *value);
  int bindin(unsigned int position, char   *value, unsigned int len);
  // Bind BLOB fields, buffer is used to store the content of the BLOB field, size is the actual size of the BLOB field.
  int bindinlob(unsigned int position, void *buffer, unsigned long *size);

  // Bind the fields of the result set to the address of the variable.
  // position: The order of the field, starting from 1, corresponds one-to-one with the fields in the SQL result set.
  // value: Address of the output variable, if it is a string, the memory size should be the length of the corresponding field plus 1.
  // len: If the data type of the output variable is a string, use len to specify its maximum length, it is recommended to use the length of the corresponding field.
  // Return value: 0-Success, Others-Failure, the program generally does not need to care about the return value.
  // Note: 1) If the SQL statement does not change, it only needs to bind once; 2) The total number of bound output variables cannot exceed MAXPARAMS.
  int bindout(unsigned int position, int    *value);
  int bindout(unsigned int position, long   *value);
  int bindout(unsigned int position, unsigned int  *value);
  int bindout(unsigned int position, unsigned long *value);
  int bindout(unsigned int position, float *value);
  int bindout(unsigned int position, double *value);
  int bindout(unsigned int position, char   *value, unsigned int len);
  // Bind BLOB fields, buffer is used to store the content of the BLOB field, buffersize is the memory size occupied by the buffer,
  // size is the actual size of the BLOB field in the result set, be sure to ensure that the buffer is large enough to prevent memory overflow.
  int bindoutlob(unsigned int position, void *buffer, unsigned long buffersize, unsigned long *size);

  // Execute SQL statement.
  // Return value: 0-Success, Others-Failure, the failure code is in m_cda.rc, the failure description is in m_cda.message.
  // If insert, update, or delete statement is successfully executed, m_cda.rpc holds the number of affected rows for this SQL execution.
  // The program must check the return value of the execute method.
  int execute();

  // Execute SQL statement.
  // If the SQL statement does not require binding of input and output variables (no bound variables, non-query statement),
  // it can be directly executed using this method.
  // Parameter explanation: This is a variable-length parameter, usage is the same as the printf function.
  // Return value: 0-Success, Others-Failure, the failure code is in m_cda.rc, the failure description is in m_cda.message,
  // if a non-query statement is successfully executed, m_cda.rpc holds the number of affected rows for this SQL execution.
  // The program must check the return value of the execute method.
  int execute(const char *fmt, ...);

  // Get one record from the result set.
  // If the executed SQL statement is a query statement, after calling the execute method, a result set (stored in the database buffer) will be generated.
  // The next method gets one record from the result set and puts the values of the fields into the bound output variables.
  // Return value: 0-Success, 1403-No more records in the result set, Others-Failure, the failure code is in m_cda.rc, the failure description is in m_cda.message.
  // The failure reasons are mainly: 1) The connection to the database has been disconnected; 2) The memory of the bound output variable is too small.
  // Each time the next method is executed, the value of m_cda.rpc is increased by 1.
  // The program must check the return value of the next method.
  int next();
};

#endif
