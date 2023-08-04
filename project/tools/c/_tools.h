#ifndef _TOOLS_H
#define _TOOLS_H

#include "_public.h"
#include "_mysql.h"

// Structure for table column information.
struct st_columns
{
  char colname[31];  // Column name.
  char datatype[31]; // Column data type, categorized as number, date, and char.
  int collen;        // Column length, number is fixed at 20, date is fixed at 19, and char length is determined by the table structure.
  int pkseq;         // If the column is a primary key field, store the sequence of the primary key field, starting from 1; otherwise, set to 0.
};

// Class to retrieve all column and primary key column information of a table.
class CTABCOLS
{
public:
  CTABCOLS();

  int m_allcount;   // Total count of all fields.
  int m_pkcount;    // Total count of primary key fields.
  int m_maxcollen;  // Maximum length among all columns, this member was added later and was not mentioned in the course.

  vector<struct st_columns> m_vallcols;  // Container to store all field information.
  vector<struct st_columns> m_vpkcols;   // Container to store primary key field information.

  char m_allcols[3001];  // String representation of all column names, separated by commas.
  char m_pkcols[301];    // String representation of primary key column names, separated by commas.

  void initdata();  // Member variable initialization.

  // Get all column information of the specified table.
  bool allcols(connection *conn, char *tablename);

  // Get primary key column information of the specified table.
  bool pkcols(connection *conn, char *tablename);
};

#endif
