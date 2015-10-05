#include "ResultData.h"
#include "IQuery.h"
#include "Database.h"
#include "string.h"
#include "Logger.h"
#include <iostream>
#include <fstream>

ResultData::ResultData(unsigned int columnCount, unsigned int rows)
{
	LOG_CURRENT_FUNCTIONCALL
	this->columnCount = columnCount;
	this->columns.resize(columnCount);
	this->columnTypes.resize(columnCount);
	this->rows.reserve(rows);
}

ResultData::ResultData() : ResultData(0,0)
{
}

ResultData::ResultData(MYSQL_RES* result) : ResultData(mysql_num_fields(result), mysql_num_rows(result))
{
	if (columnCount == 0) return;
	for (unsigned int i = 0; i < columnCount; i++)
	{
		MYSQL_FIELD *field = mysql_fetch_field_direct(result, i);
		columnTypes[i] = field->type;
		columns[i] = field->name;
	}
	MYSQL_ROW currentRow;
	//This shouldn't error since mysql_store_results stores ALL rows already
	while ((currentRow = mysql_fetch_row(result)) != NULL)
	{
		unsigned long *lengths = mysql_fetch_lengths(result);
		this->rows.emplace_back(lengths, currentRow, columnCount);
	}
}

static bool mysqlStmtFetch(MYSQL_STMT* stmt)
{
	int result = mysql_stmt_fetch(stmt);
	if (result == 0) return true;
	if (result == 1)
	{
		const char* errorMessage = mysql_stmt_error(stmt);
		int errorCode = mysql_stmt_errno(stmt);
		throw MySQLException(errorCode, errorMessage);
	}
	else
	{
		return false;
	}
}
static void mysqlStmtBindResult(MYSQL_STMT* stmt, MYSQL_BIND* bind)
{
	int result = mysql_stmt_bind_result(stmt, bind);
	if (result != 0)
	{
		const char* errorMessage = mysql_stmt_error(stmt);
		int errorCode = mysql_stmt_errno(stmt);
		throw MySQLException(errorCode, errorMessage);
	}
}

ResultData::ResultData(MYSQL_STMT* result) : ResultData(mysql_stmt_field_count(result), mysql_stmt_num_rows(result))
{
	if (this->columnCount == 0) return;
	MYSQL_RES * metaData = mysql_stmt_result_metadata(result);
	if (metaData == NULL){ throw std::runtime_error("mysql_stmt_result_metadata: Unknown Error"); }
	MYSQL_FIELD *fields = mysql_fetch_fields(metaData);
	std::vector<MYSQL_BIND> binds(columnCount);
	std::vector<my_bool> isFieldNull(columnCount);
	std::vector<std::vector<char>> buffers;
	std::vector<unsigned long> lengths(columnCount);
	for (unsigned int i = 0; i < columnCount; i++)
	{
		columnTypes[i] = fields[i].type;
		columns[i] = fields[i].name;
		MYSQL_BIND& bind = binds[i];
		bind.buffer_type = MYSQL_TYPE_STRING;
		buffers.emplace_back(fields[i].max_length + 2);
		bind.buffer = buffers.back().data();
		bind.buffer_length = fields[i].max_length+1;
		bind.length = &lengths[i];
		bind.is_null = &isFieldNull[i];
		bind.is_unsigned = 0;
	}
	mysqlStmtBindResult(result, binds.data());
	while (mysqlStmtFetch(result))
	{
		this->rows.emplace_back(result, binds.data(), columnCount);
	}
}

ResultData::~ResultData()
{
}

ResultDataRow::ResultDataRow(unsigned int columnCount)
{
	this->columnCount = columnCount;
	this->values.resize(columnCount);
	this->nullFields.resize(columnCount);
}

ResultDataRow::ResultDataRow(unsigned long *lengths, MYSQL_ROW row, unsigned int columnCount) : ResultDataRow(columnCount)
{
	for (unsigned int i = 0; i < columnCount; i++)
	{
		if (row[i])
		{
			this->values[i] = std::string(row[i], lengths[i]);
		}
		else
		{
			if (lengths[i] == 0)
			{
				this->nullFields[i] = true;
			}
			this->values[i] = "";
		}
	}
}

ResultDataRow::ResultDataRow(MYSQL_STMT* statement, MYSQL_BIND* bind, unsigned int columnCount) : ResultDataRow(columnCount)
{
	for (unsigned int i = 0; i < columnCount; i++)
	{
		if (!*(bind[i].is_null) && bind[i].buffer)
		{
			this->values[i] = std::string((char*)bind[i].buffer, *bind[i].length);
		}
		else
		{
			if (*(bind[i].is_null))
			{
				this->nullFields[i] = true;
			}
			this->values[i] = "";
		}
	}
}