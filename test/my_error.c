/*
  Copyright (C) 1997-2007 MySQL AB

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  There are special exceptions to the terms and conditions of the GPL
  as it is applied to this software. View the full text of the exception
  in file LICENSE.exceptions in the top-level directory of this software
  distribution.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "odbctap.h"


int check_sqlstate(SQLHDBC hdbc, SQLHSTMT hstmt, char *sqlstate)
{
  SQLCHAR     sql_state[6];
  SQLINTEGER  err_code= 0;
  SQLCHAR     err_msg[SQL_MAX_MESSAGE_LENGTH]= {0};
  SQLSMALLINT err_len= 0;

  memset(err_msg, 'C', SQL_MAX_MESSAGE_LENGTH);
  SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sql_state, &err_code, err_msg,
                SQL_MAX_MESSAGE_LENGTH - 1, &err_len);

  if (!driver_min_version(hdbc, (SQLCHAR *)"03.52",5))
    is_str(sql_state, (SQLCHAR *)sqlstate, 5);

  return OK;
}


DECLARE_TEST(t_odbc3_error)
{
  SQLHENV henv1;
  SQLHDBC hdbc1;
  SQLHSTMT hstmt1;
  SQLINTEGER ov_version;

  ok_env(henv1, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv1));
  ok_env(henv1, SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)SQL_OV_ODBC3, 0));

  ok_env(henv1, SQLAllocHandle(SQL_HANDLE_DBC, henv1, &hdbc1));

  ok_env(henv1, SQLGetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)&ov_version, 0, 0));
  is_num(ov_version, SQL_OV_ODBC3);

  ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS, myuid, SQL_NTS,
                           mypwd, SQL_NTS));

  ok_con(hdbc1, SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt1));

  expect_sql(hstmt1, "SELECT * FROM non_existing_table", SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "42S02") != OK)
    return FAIL;

  ok_sql(hstmt1, "DROP TABLE IF EXISTS t_error");
  ok_sql(hstmt1, "CREATE TABLE t_error (id INT)");

  expect_sql(hstmt1, "CREATE TABLE t_error (id INT)", SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "42S01") != OK)
    return FAIL;

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  expect_stmt(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_FETCH_BOOKMARK_PTR,
                                     (SQLPOINTER)NULL, 0),
              SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "HYC00") != OK)
    return FAIL;

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  ok_sql(hstmt1, "DROP TABLE IF EXISTS t_error");

  ok_con(hdbc1, SQLDisconnect(hdbc1));

  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  ok_env(henv1, SQLFreeHandle(SQL_HANDLE_ENV, henv1));

  return OK;
}


DECLARE_TEST(t_odbc2_error)
{
  SQLHENV henv1;
  SQLHDBC hdbc1;
  SQLHSTMT hstmt1;
  SQLINTEGER ov_version;

  ok_env(henv1, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv1));
  ok_env(henv1, SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)SQL_OV_ODBC2, 0));

  ok_env(henv1, SQLAllocHandle(SQL_HANDLE_DBC, henv1, &hdbc1));

  ok_env(henv1, SQLGetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)&ov_version, 0, 0));
  is_num(ov_version, SQL_OV_ODBC2);

  ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS, myuid, SQL_NTS,
                           mypwd, SQL_NTS));

  ok_con(hdbc1, SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt1));

  expect_sql(hstmt1, "SELECT * FROM non_existing_table", SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "S0002") != OK)
    return FAIL;

  ok_sql(hstmt1, "DROP TABLE IF EXISTS t_error");
  ok_sql(hstmt1, "CREATE TABLE t_error (id INT)");

  expect_sql(hstmt1, "CREATE TABLE t_error (id INT)", SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "S0001") != OK)
    return FAIL;

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  expect_stmt(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_FETCH_BOOKMARK_PTR,
                                     (SQLPOINTER)NULL, 0),
              SQL_ERROR);
  if (check_sqlstate(hdbc1, hstmt1, "S1C00") != OK)
    return FAIL;

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  ok_sql(hstmt1, "DROP TABLE IF EXISTS t_error");

  ok_con(hdbc1, SQLDisconnect(hdbc1));

  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  ok_env(henv1, SQLFreeHandle(SQL_HANDLE_ENV, henv1));

  return OK;
}

static int test_diagrec(SQLSMALLINT HandleType,SQLHANDLE Handle,
                        SQLSMALLINT RecNumber, SQLSMALLINT BufferLength,
                        SQLRETURN return_value_expected)
{
  SQLRETURN rc;
  SQLCHAR   sqlstate[6]= {0};
  SQLCHAR   message[255]= {0};
  SQLINTEGER native_err= 0;
  SQLSMALLINT msglen= 0;

  rc= SQLGetDiagRec(HandleType, Handle, RecNumber, sqlstate, &native_err,
                    message, BufferLength, &msglen);

  is_num(rc, return_value_expected);

  return OK;
}


DECLARE_TEST(t_diagrec)
{
  expect_sql(hstmt, "DROP TABLE t_odbc3_non_existent_table", SQL_ERROR);

  if (test_diagrec(SQL_HANDLE_STMT, hstmt, 2, 0,   SQL_NO_DATA_FOUND) != OK)
    return FAIL;
  if (test_diagrec(SQL_HANDLE_STMT, hstmt, 1, 255, SQL_SUCCESS) != OK)
    return FAIL;
  if (test_diagrec(SQL_HANDLE_STMT, hstmt, 1, 0,   SQL_SUCCESS_WITH_INFO) != OK)
    return FAIL;
  if (test_diagrec(SQL_HANDLE_STMT, hstmt, 1, 10,  SQL_SUCCESS_WITH_INFO) != OK)
    return FAIL;
  if (test_diagrec(SQL_HANDLE_STMT, hstmt, 1, -1,  SQL_ERROR) != OK)
    return FAIL;

  return OK;
}


DECLARE_TEST(t_warning)
{
  SQLCHAR    szData[20];
  SQLLEN     pcbValue;

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_warning");
  ok_sql(hstmt, "CREATE TABLE t_warning (col2 CHAR(20))");
  ok_sql(hstmt, "INSERT INTO t_warning VALUES ('Venu Anuganti')");

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0));

  /* ignore all columns */
  ok_sql(hstmt, "SELECT * FROM t_warning");

  ok_stmt(hstmt, SQLFetch(hstmt));

  expect_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, szData, 4, &pcbValue),
              SQL_SUCCESS_WITH_INFO);
  is_str(szData, "Ven", 3);
  is_num(pcbValue, 13);

  expect_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, szData, 4, &pcbValue),
              SQL_SUCCESS_WITH_INFO);
  is_str(szData, "u A", 3);
  is_num(pcbValue, 10);

  expect_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, szData, 4, &pcbValue),
              SQL_SUCCESS_WITH_INFO);
  is_str(szData, "nug", 3);
  is_num(pcbValue, 7);

  expect_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, szData, 4, &pcbValue),
              SQL_SUCCESS_WITH_INFO);
  is_str(szData, "ant", 3);
  is_num(pcbValue, 4);

  expect_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, szData, 4, &pcbValue),
              SQL_SUCCESS);
  is_str(szData, "i", 1);
  is_num(pcbValue, 1);

  expect_stmt(hstmt, SQLFetch(hstmt), SQL_NO_DATA_FOUND);

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_UNBIND));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  return OK;
}


BEGIN_TESTS
  ADD_TEST(t_odbc2_error)
  ADD_TEST(t_odbc3_error)
  /* Run twice to test the driver's handling of switching  */
  ADD_TEST(t_odbc2_error)
  ADD_TEST(t_diagrec)
  ADD_TEST(t_warning)
END_TESTS

RUN_TESTS
