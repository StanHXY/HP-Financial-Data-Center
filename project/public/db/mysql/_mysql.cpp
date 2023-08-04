/**************************************************************************************/
/* Program Name: _mysql.cpp, this program is the definition file for C/C++ operations on the MySQL database in the development framework. */
/**************************************************************************************/

#include "_mysql.h"

// Connection Class Constructor
connection::connection()
{
    m_conn = NULL;
    m_state = 0;
    memset(&m_env, 0, sizeof(LOGINENV));
    memset(&m_cda, 0, sizeof(m_cda));
    m_cda.rc = -1;
    strncpy(m_cda.message, "database not open.", 128);
    memset(m_dbtype, 0, sizeof(m_dbtype));
    strcpy(m_dbtype, "mysql"); // Database type
}

// Connection Class Destructor
connection::~connection()
{
    disconnect();
}

// Parse username, password, and tnsname from connstr
// "120.77.115.3","szidc","SZmb1601","lxqx",3306
void connection::setdbopt(const char* connstr)
{
    memset(&m_env, 0, sizeof(LOGINENV));
    char* bpos, * epos;
    bpos = epos = 0;

    // IP
    bpos = (char*)connstr;
    epos = strstr(bpos, ",");
    if (epos > 0)
    {
        strncpy(m_env.ip, bpos, epos - bpos);
    }
    else
        return;

    // User
    bpos = epos + 1;
    epos = 0;
    epos = strstr(bpos, ",");
    if (epos > 0)
    {
        strncpy(m_env.user, bpos, epos - bpos);
    }
    else
        return;

    // Password
    bpos = epos + 1;
    epos = 0;
    epos = strstr(bpos, ",");
    if (epos > 0)
    {
        strncpy(m_env.pass, bpos, epos - bpos);
    }
    else
        return;

    // DB name
    bpos = epos + 1;
    epos = 0;
    epos = strstr(bpos, ",");
    if (epos > 0)
    {
        strncpy(m_env.dbname, bpos, epos - bpos);
    }
    else
        return;

    // Port
    m_env.port = atoi(epos + 1);
}

// Connect to the database using the provided connection string, charset, and autocommit option.
int connection::connecttodb(const char* connstr, const char* charset, unsigned int autocommitopt)
{
    // If already connected to the database, return success.
    // To reconnect, the disconnect() method must be called explicitly first.
    if (m_state == 1)
        return 0;

    // Parse username, password, and tnsname from connstr
    setdbopt(connstr);

    memset(&m_cda, 0, sizeof(m_cda));

    if ((m_conn = mysql_init(NULL)) == NULL)
    {
        m_cda.rc = -1;
        strncpy(m_cda.message, "initialize mysql failed.\n", 128);
        return -1;
    }

    if (mysql_real_connect(m_conn, m_env.ip, m_env.user, m_env.pass, m_env.dbname, m_env.port, NULL, 0) == NULL)
    {
        m_cda.rc = mysql_errno(m_conn);
        strncpy(m_cda.message, mysql_error(m_conn), 2000);
        mysql_close(m_conn);
        m_conn = NULL;
        return -1;
    }

    // Set transaction mode, 0-disable autocommit, 1-enable autocommit
    m_autocommitopt = autocommitopt;

    if (mysql_autocommit(m_conn, m_autocommitopt) != 0)
    {
        m_cda.rc = mysql_errno(m_conn);
        strncpy(m_cda.message, mysql_error(m_conn), 2000);
        mysql_close(m_conn);
        m_conn = NULL;
        return -1;
    }

    // Set character set, should be consistent with the database to avoid garbled characters
    character(charset);

    m_state = 1;

    // Set transaction isolation level to read committed
    execute("set session transaction isolation level read committed");

    return 0;
}

// Set character set, should be consistent with the database to avoid garbled characters
void connection::character(const char* charset)
{
    if (charset == 0)
        return;

    mysql_set_character_set(m_conn, charset);

    return;
}


int connection::disconnect()
{
    memset(&m_cda, 0, sizeof(m_cda));

    if (m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.", 128); return -1;
    }

    rollback();

    mysql_close(m_conn);

    m_conn = NULL;

    m_state = 0;

    return 0;
}

int connection::rollback()
{
    memset(&m_cda, 0, sizeof(m_cda));

    if (m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.", 128); return -1;
    }

    if (mysql_rollback(m_conn) != 0)
    {
        m_cda.rc = mysql_errno(m_conn); strncpy(m_cda.message, mysql_error(m_conn), 2000); mysql_close(m_conn); m_conn = NULL; return -1;
    }

    return 0;
}

int connection::commit()
{
    memset(&m_cda, 0, sizeof(m_cda));

    if (m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.", 128); return -1;
    }

    if (mysql_commit(m_conn) != 0)
    {
        m_cda.rc = mysql_errno(m_conn); strncpy(m_cda.message, mysql_error(m_conn), 2000); mysql_close(m_conn); m_conn = NULL; return -1;
    }

    return 0;
}

void connection::err_report()
{
    if (m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.", 128); return;
    }

    memset(&m_cda, 0, sizeof(m_cda));

    m_cda.rc = -1;
    strncpy(m_cda.message, "call err_report failed.", 128);

    m_cda.rc = mysql_errno(m_conn);

    strncpy(m_cda.message, mysql_error(m_conn), 2000);

    return;
}

sqlstatement::sqlstatement()
{
    initial();
}

void sqlstatement::initial()
{
    m_state = 0;

    m_handle = NULL;

    memset(&m_cda, 0, sizeof(m_cda));

    memset(m_sql, 0, sizeof(m_sql));

    m_cda.rc = -1;
    strncpy(m_cda.message, "sqlstatement not connect to connection.\n", 128);
}

sqlstatement::sqlstatement(connection *conn)
{
    initial();

    connect(conn);
}

sqlstatement::~sqlstatement()
{
    disconnect();
}

int sqlstatement::connect(connection *conn)
{
    // Note that an sqlstatement can only be bound to one connection in the program, 
    // and it is not allowed to bind to multiple connections. 
    // So, if this sqlstatement is already bound to a connection, return success directly.
    if (m_state == 1)
        return 0;

    memset(&m_cda, 0, sizeof(m_cda));

    m_conn = conn;

    // If the database connection class pointer is empty, return failure directly.
    if (m_conn == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.\n", 128); return -1;
    }

    // If the database is not connected properly, return failure directly.
    if (m_conn->m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "database not open.\n", 128); return -1;
    }

    if ((m_handle = mysql_stmt_init(m_conn->m_conn)) == NULL)
    {
        err_report(); return m_cda.rc;
    }

    m_state = 1;

    m_autocommitopt = m_conn->m_autocommitopt;

    return 0;
}

int sqlstatement::disconnect()
{
    if (m_state == 0)
        return 0;

    memset(&m_cda, 0, sizeof(m_cda));

    mysql_stmt_close(m_handle);

    m_state = 0;

    m_handle = NULL;

    memset(&m_cda, 0, sizeof(m_cda));

    memset(m_sql, 0, sizeof(m_sql));

    m_cda.rc = -1;
    strncpy(m_cda.message, "cursor not open.", 128);

    return 0;
}

int connection::execute(const char *fmt, ...)
{
    memset(m_sql, 0, sizeof(m_sql));

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_sql, 10240, fmt, ap);
    va_end(ap);

    sqlstatement stmt(this);

    return stmt.execute(m_sql);
}

void sqlstatement::err_report()
{
    // Note that in this function, do not use memset(&m_cda, 0, sizeof(m_cda)) carelessly, 
    // otherwise, the content of m_cda.rpc will be cleared.
    if (m_state == 0)
    {
        m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return;
    }

    memset(&m_conn->m_cda, 0, sizeof(m_conn->m_cda));

    m_cda.rc = -1;
    strncpy(m_cda.message, "call err_report() failed.\n", 128);

    m_cda.rc = mysql_stmt_errno(m_handle);

    snprintf(m_cda.message, 2000, "%d,%s", m_cda.rc, mysql_stmt_error(m_handle));

    m_conn->err_report();

    return;
}

// Convert lowercase letters in a string to uppercase, ignoring non-letter characters.
// This function is only used in the prepare method.
void MY__ToUpper(char *str)
{
    if (str == 0) return;

    if (strlen(str) == 0) return;

    int istrlen = strlen(str);

    for (int ii = 0; ii < istrlen; ii++)
    {
        if ((str[ii] >= 'a') && (str[ii] <= 'z')) str[ii] = str[ii] - 32;
    }
}

// Remove leading spaces from a string.
// This function is only used in the prepare method.
void MY__DeleteLChar(char *str, const char chr)
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

// String replacement function
// In the string "str", if "str1" exists, replace it with "str2".
// This function is only used in the prepare method.
void MY__UpdateStr(char *str, const char *str1, const char *str2, bool bloop)
{
  if (str == 0) return;
  if (strlen(str) == 0) return;
  if ((str1 == 0) || (str2 == 0)) return;

  // If bloop is true and str2 contains the content of str1, directly return to avoid entering an infinite loop, which may lead to memory overflow.
  if ((bloop == true) && (strstr(str2, str1) > 0)) return;

  // Allocate more space if possible, but there may still be a risk of memory overflow. It's better to optimize it using a string.
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
    strncpy(strTemp, str, strPos - str);
    strcat(strTemp, str2);
    strcat(strTemp, strPos + strlen(str1));
    strcpy(str, strTemp);

    strStart = strPos + strlen(str2);
  }
}

int sqlstatement::prepare(const char *fmt, ...)
{
  memset(&m_cda, 0, sizeof(m_cda));

  if (m_state == 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return -1;
  }

  memset(m_sql, 0, sizeof(m_sql));

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(m_sql, 10240, fmt, ap);
  va_end(ap);

  // To be compatible with Oracle, replace ":1", ":2", ":3", etc. with "?"
  char strtmp[11];
  for (int ii = MAXPARAMS; ii > 0; ii--)
  {
    memset(strtmp, 0, sizeof(strtmp));
    snprintf(strtmp, 10, ":%d", ii);
    MY__UpdateStr(m_sql, strtmp, "?", false);
  }

  // To be compatible with Oracle
  // Replace "to_date" with "str_to_date".
  if (strstr(m_sql, "str_to_date") == 0) MY__UpdateStr(m_sql, "to_date", "str_to_date", false);
  // Replace "to_char" with "date_format".
  MY__UpdateStr(m_sql, "to_char", "date_format", false);
  // Replace "yyyy-mm-dd hh24:mi:ss" with "%Y-%m-%d %H:%i:%s"
  MY__UpdateStr(m_sql, "yyyy-mm-dd hh24:mi:ss", "%Y-%m-%d %H:%i:%s", false);
  // Replace "yyyymmddhh24miss" with "%Y%m%d%H%i%s"
  MY__UpdateStr(m_sql, "yyyymmddhh24miss", "%Y%m%d%H%i%s", false);
  // If you want to be compatible with more date formats of Oracle and MySQL, you can add code here.
  // Be sure to list each format one by one, and avoid replacing "yyyy" with "%Y", as "yyyy" may also exist in other parts of the SQL statement.

  // Replace "sysdate" with "sysdate()".
  if (strstr(m_sql, "sysdate()") == 0) MY__UpdateStr(m_sql, "sysdate", "sysdate()", false);

  if (mysql_stmt_prepare(m_handle, m_sql, strlen(m_sql)) != 0)
  {
    err_report(); memcpy(&m_cda1, &m_cda, sizeof(struct CDA_DEF)); return m_cda.rc;
  }

  // Determine whether it is a query statement (m_sqltype=0) or other types of statements (m_sqltype=1).
  m_sqltype = 1;

  // Extract the first 30 characters from the SQL statement to check if it starts with "select", which indicates a query statement.
  char strtemp[31]; memset(strtemp, 0, sizeof(strtemp)); strncpy(strtemp, m_sql, 30);
  MY__ToUpper(strtemp); MY__DeleteLChar(strtemp, ' ');
  if (strncmp(strtemp, "SELECT", 6) == 0) m_sqltype = 0;

  memset(params_in, 0, sizeof(params_in));

  memset(params_out, 0, sizeof(params_out));

  maxbindin = 0;

  return 0;
}

int sqlstatement::bindin(unsigned int position, int *value)
{
  if (m_state == 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return -1;
  }

  if ((position < 1) || (position >= MAXPARAMS) || (position > m_handle->param_count))
  {
    m_cda.rc = -1; strncpy(m_cda.message, "array bound.", 128);
  }

  params_in[position - 1].buffer_type = MYSQL_TYPE_LONG;
  params_in[position - 1].buffer = value;

  if (position > maxbindin) maxbindin = position;

  return 0;
}

int sqlstatement::bindin(unsigned int position, long *value)
{
  if (m_state == 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return -1;
  }

  if ((position < 1) || (position >= MAXPARAMS) || (position > m_handle->param_count))
  {
    m_cda.rc = -1; strncpy(m_cda.message, "array bound.", 128);
  }

  params_in[position - 1].buffer_type = MYSQL_TYPE_LONGLONG;
  params_in[position - 1].buffer = value;

  if (position > maxbindin) maxbindin = position;

  return 0;
}

int sqlstatement::bindin(unsigned int position, unsigned int *value)
{
  if (m_state == 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return -1;
  }

  if ((position < 1) || (position >= MAXPARAMS) || (position > m_handle->param_count))
  {
    m_cda.rc = -1; strncpy(m_cda.message, "array bound.", 128);
  }

  params_in[position - 1].buffer_type = MYSQL_TYPE_LONG;
  params_in[position - 1].buffer = value;

  if (position > maxbindin) maxbindin = position;

  return 0;
}


int sqlstatement::bindin(unsigned int position,unsigned long *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->param_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_in[position-1].buffer_type = MYSQL_TYPE_LONGLONG;
  params_in[position-1].buffer = value;

  if (position>maxbindin) maxbindin=position;

  return 0;
}

int sqlstatement::bindin(unsigned int position,char *value,unsigned int len)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->param_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_in[position-1].buffer_type = MYSQL_TYPE_VAR_STRING;
  params_in[position-1].buffer = value;
  params_in[position-1].length=&params_in_length[position-1];
  params_in[position-1].is_null=&params_in_is_null[position-1];

  if (position>maxbindin) maxbindin=position;

  return 0;
}

int sqlstatement::bindinlob(unsigned int position,void *buffer,unsigned long *size)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->param_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_in[position-1].buffer_type = MYSQL_TYPE_BLOB;
  params_in[position-1].buffer = buffer;
  params_in[position-1].length=size;
  params_in[position-1].is_null=&params_in_is_null[position-1];

  if (position>maxbindin) maxbindin=position;

  return 0;
}

int sqlstatement::bindin(unsigned int position,float *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->param_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_in[position-1].buffer_type = MYSQL_TYPE_FLOAT;
  params_in[position-1].buffer = value;

  if (position>maxbindin) maxbindin=position;

  return 0;
}

int sqlstatement::bindin(unsigned int position,double *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->param_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_in[position-1].buffer_type = MYSQL_TYPE_DOUBLE;
  params_in[position-1].buffer = value;

  if (position>maxbindin) maxbindin=position;

  return 0;
}

///////////////////
int sqlstatement::bindout(unsigned int position,int *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_LONG;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::bindout(unsigned int position,long *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_LONGLONG;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::bindout(unsigned int position,unsigned int *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_LONG;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::bindout(unsigned int position,unsigned long *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_LONGLONG;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::bindout(unsigned int position,char *value,unsigned int len)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_VAR_STRING;
  params_out[position-1].buffer = value;
  params_out[position-1].buffer_length = len;

  return 0;
}

int sqlstatement::bindoutlob(unsigned int position,void *buffer,unsigned long buffersize,unsigned long *size)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_BLOB;
  params_out[position-1].length = size;
  params_out[position-1].buffer = buffer;
  params_out[position-1].buffer_length = buffersize;

  return 0;
}


int sqlstatement::bindout(unsigned int position,float *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_FLOAT;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::bindout(unsigned int position,double *value)
{
  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (position<1) || (position>=MAXPARAMS) || (position>m_handle->field_count) )
  {
    m_cda.rc=-1; strncpy(m_cda.message,"array bound.",128);
  }

  params_out[position-1].buffer_type = MYSQL_TYPE_DOUBLE;
  params_out[position-1].buffer = value;

  return 0;
}

int sqlstatement::execute()
{
  memset(&m_cda,0,sizeof(m_cda));

  if (m_state == 0)
  {
    m_cda.rc=-1; strncpy(m_cda.message,"cursor not open.\n",128); return -1;
  }

  if ( (m_handle->param_count>0) && (m_handle->bind_param_done == 0))
  {
    if (mysql_stmt_bind_param(m_handle,params_in) != 0)
    {
      err_report(); return m_cda.rc;
    }
  }

  if ( (m_handle->field_count>0) && (m_handle->bind_result_done == 0) )
  {
    if (mysql_stmt_bind_result(m_handle,params_out) != 0)
    {
      err_report(); return m_cda.rc;
    }
  }
  
  // Handle the case where string fields are empty.
for (int ii = 0; ii < maxbindin; ii++)
{
  if (params_in[ii].buffer_type == MYSQL_TYPE_VAR_STRING)
  {
    if (strlen((char *)params_in[ii].buffer) == 0)
    {
      params_in_is_null[ii] = true;
    }
    else
    {
      params_in_is_null[ii] = false;
      params_in_length[ii] = strlen((char *)params_in[ii].buffer);
    }
  }

  if (params_in[ii].buffer_type == MYSQL_TYPE_BLOB)
  {
    if ((*params_in[ii].length) == 0)
      params_in_is_null[ii] = true;
    else
      params_in_is_null[ii] = false;
  }
}

if (mysql_stmt_execute(m_handle) != 0)
{
  err_report();

  if (m_cda.rc == 1243) memcpy(&m_cda, &m_cda1, sizeof(struct CDA_DEF));

  return m_cda.rc;
}

// If it is not a query statement, get the number of affected rows.
if (m_sqltype == 1)
{
  m_cda.rpc = m_handle->affected_rows;
  m_conn->m_cda.rpc = m_cda.rpc;
}

/*
if (m_sqltype == 0)
  mysql_store_result(m_conn->m_conn);
*/

return 0;
}

int sqlstatement::execute(const char *fmt, ...)
{
  char strtmpsql[10241];
  memset(strtmpsql, 0, sizeof(strtmpsql));

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(strtmpsql, 10240, fmt, ap);
  va_end(ap);

  if (prepare(strtmpsql) != 0) return m_cda.rc;

  return execute();
}

int sqlstatement::next()
{
  // Note: Do not use memset(&m_cda, 0, sizeof(m_cda)) in this function as it will clear the m_cda.rpc.
  if (m_state == 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "cursor not open.\n", 128); return -1;
  }

  // If the statement was not executed successfully, return failure.
  if (m_cda.rc != 0) return m_cda.rc;

  // Check if it is a query statement, if not, return an error.
  if (m_sqltype != 0)
  {
    m_cda.rc = -1; strncpy(m_cda.message, "no recordset found.\n", 128); return -1;
  }

  int ret = mysql_stmt_fetch(m_handle);

  if (ret == 0)
  {
    m_cda.rpc++; return 0;
  }

  if (ret == 1)
  {
    err_report(); return m_cda.rc;
  }

  if (ret == MYSQL_NO_DATA) return MYSQL_NO_DATA;

  if (ret == MYSQL_DATA_TRUNCATED)
  {
    m_cda.rpc++; return 0;
  }

  return 0;
}

// Load the file "filename" into the buffer, make sure the buffer is large enough.
// Return the size of the file on success, return 0 if the file does not exist or is empty.
unsigned long filetobuf(const char *filename, char *buffer)
{
  FILE *fp;
  int bytes = 0;
  int total_bytes = 0;

  if ((fp = fopen(filename, "r")) == 0) return 0;

  while (true)
  {
    bytes = fread(buffer + total_bytes, 1, 5000, fp);

    total_bytes = total_bytes + bytes;

    if (bytes < 5000) break;
  }

  fclose(fp);

  return total_bytes;
}

// Write the content of the buffer to the file "filename", with "size" being the size of the valid content in the buffer.
// Return true on success, false on failure.
bool buftofile(const char *filename, char *buffer, unsigned long size)
{
  if (size == 0) return false;

  char filenametmp[301];
  memset(filenametmp, 0, sizeof(filenametmp));
  snprintf(filenametmp, 300, "%s.tmp", filename);

  FILE *fp;

  if ((fp = fopen(filenametmp, "w")) == 0) return false;

  // If the buffer is large, there might be a situation where one write is not enough. The following code can be optimized to use a loop to write.
  size_t tt = fwrite(buffer, 1, size, fp);

  if (tt != size)
  {
    remove(filenametmp); return false;
  }

  fclose(fp);

  if (rename(filenametmp, filename) != 0) return false;

  return true;
}
