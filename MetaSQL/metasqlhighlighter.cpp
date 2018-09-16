/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2014 by OpenMFG, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include "metasqlhighlighter.h"

#include <QColor>
#include <QTextEdit>

// #include "format.h"

static QStringList _keyword;
static QStringList _extension;

/* TODO: extract the colors to use from the locale
static QColor error("red");
static QColor warning("orange");
static QColor emphasis("blue");
static QColor altemphasis("green");
static QColor expired("red");
static QColor future("blue");
 */

void MetaSQLHighlighter::init()
{
  /* TODO: extract the colors to use from the locale */
  _commentColor   = QColor("green");    // namedColor("altemphasis");
  _errorColor     = QColor("red");      // namedColor("error");
  _extensionColor = QColor("orange");   // namedColor("warning");
  _keywordColor   = QColor("blue");     // namedColor("emphasis");
  _literalColor   = QColor("purple");   // namedColor("future");

  // taken from postgresql docs "SQL Key Words"
  if (_keyword.isEmpty())
    _keyword  << "A"
              << "ABORT"
              << "ABS"
              << "ABSOLUTE"
              << "ACCESS"
              << "ACTION"
              << "ADA"
              << "ADD"
              << "ADMIN"
              << "AFTER"
              << "AGGREGATE"
              << "ALIAS"
              << "ALL"
              << "ALLOCATE"
              << "ALSO"
              << "ALTER"
              << "ALWAYS"
              << "ANALYSE"
              << "ANALYZE"
              << "AND"
              << "ANY"
              << "ARE"
              << "ARRAY"
              << "AS"
              << "ASC"
              << "ASENSITIVE"
              << "ASSERTION"
              << "ASSIGNMENT"
              << "ASYMMETRIC"
              << "AT"
              << "ATOMIC"
              << "ATTRIBUTE"
              << "ATTRIBUTES"
              << "AUTHORIZATION"
              << "AVG"
              << "BACKWARD"
              << "BEFORE"
              << "BEGIN"
              << "BERNOULLI"
              << "BETWEEN"
              << "BIGINT"
              << "BINARY"
              << "BIT"
              << "BITVAR"
              << "BIT_LENGTH"
              << "BLOB"
              << "BOOLEAN"
              << "BOTH"
              << "BREADTH"
              << "BY"
              << "C"
              << "CACHE"
              << "CALL"
              << "CALLED"
              << "CARDINALITY"
              << "CASCADE"
              << "CASCADED"
              << "CASE"
              << "CAST"
              << "CATALOG"
              << "CATALOG_NAME"
              << "CEIL"
              << "CEILING"
              << "CHAIN"
              << "CHAR"
              << "CHARACTER"
              << "CHARACTERISTICS"
              << "CHARACTERS"
              << "CHARACTER_LENGTH"
              << "CHARACTER_SET_CATALOG"
              << "CHARACTER_SET_NAME"
              << "CHARACTER_SET_SCHEMA"
              << "CHAR_LENGTH"
              << "CHECK"
              << "CHECKED"
              << "CHECKPOINT"
              << "CLASS"
              << "CLASS_ORIGIN"
              << "CLOB"
              << "CLOSE"
              << "CLUSTER"
              << "COALESCE"
              << "COBOL"
              << "COLLATE"
              << "COLLATION"
              << "COLLATION_CATALOG"
              << "COLLATION_NAME"
              << "COLLATION_SCHEMA"
              << "COLLECT"
              << "COLUMN"
              << "COLUMN_NAME"
              << "COMMAND_FUNCTION"
              << "COMMAND_FUNCTION_CODE"
              << "COMMENT"
              << "COMMIT"
              << "COMMITTED"
              << "COMPLETION"
              << "CONCURRENTLY"
              << "CONDITION"
              << "CONDITION_NUMBER"
              << "CONNECT"
              << "CONNECTION"
              << "CONNECTION_NAME"
              << "CONSTRAINT"
              << "CONSTRAINTS"
              << "CONSTRAINT_CATALOG"
              << "CONSTRAINT_NAME"
              << "CONSTRAINT_SCHEMA"
              << "CONSTRUCTOR"
              << "CONTAINS"
              << "CONTINUE"
              << "CONVERSION"
              << "CONVERT"
              << "COPY"
              << "CORR"
              << "CORRESPONDING"
              << "COUNT"
              << "COVAR_POP"
              << "COVAR_SAMP"
              << "CREATE"
              << "CREATEDB"
              << "CREATEROLE"
              << "CREATEUSER"
              << "CROSS"
              << "CSV"
              << "CUBE"
              << "CUME_DIST"
              << "CURRENT"
              << "CURRENT_DATE"
              << "CURRENT_DEFAULT_TRANSFORM_GROUP"
              << "CURRENT_PATH"
              << "CURRENT_ROLE"
              << "CURRENT_TIME"
              << "CURRENT_TIMESTAMP"
              << "CURRENT_TRANSFORM_GROUP_FOR_TYPE"
              << "CURRENT_USER"
              << "CURSOR"
              << "CURSOR_NAME"
              << "CYCLE"
              << "DATA"
              << "DATABASE"
              << "DATE"
              << "DATETIME_INTERVAL_CODE"
              << "DATETIME_INTERVAL_PRECISION"
              << "DAY"
              << "DEALLOCATE"
              << "DEC"
              << "DECIMAL"
              << "DECLARE"
              << "DEFAULT"
              << "DEFAULTS"
              << "DEFERRABLE"
              << "DEFERRED"
              << "DEFINED"
              << "DEFINER"
              << "DEGREE"
              << "DELETE"
              << "DELIMITER"
              << "DELIMITERS"
              << "DENSE_RANK"
              << "DEPTH"
              << "DEREF"
              << "DERIVED"
              << "DESC"
              << "DESCRIBE"
              << "DESCRIPTOR"
              << "DESTROY"
              << "DESTRUCTOR"
              << "DETERMINISTIC"
              << "DIAGNOSTICS"
              << "DICTIONARY"
              << "DISABLE"
              << "DISCONNECT"
              << "DISPATCH"
              << "DISTINCT"
              << "DO"
              << "DOMAIN"
              << "DOUBLE"
              << "DROP"
              << "DYNAMIC"
              << "DYNAMIC_FUNCTION"
              << "DYNAMIC_FUNCTION_CODE"
              << "EACH"
              << "ELEMENT"
              << "ELSE"
              << "ENABLE"
              << "ENCODING"
              << "ENCRYPTED"
              << "END"
              << "END-EXEC"
              << "EQUALS"
              << "ESCAPE"
              << "EVERY"
              << "EXCEPT"
              << "EXCEPTION"
              << "EXCLUDE"
              << "EXCLUDING"
              << "EXCLUSIVE"
              << "EXEC"
              << "EXECUTE"
              << "EXISTING"
              << "EXISTS"
              << "EXP"
              << "EXPLAIN"
              << "EXTERNAL"
              << "EXTRACT"
              << "false"
              << "FETCH"
              << "FILTER"
              << "FINAL"
              << "FIRST"
              << "FLOAT"
              << "FLOOR"
              << "FOLLOWING"
              << "FOR"
              << "FORCE"
              << "FOREIGN"
              << "FORTRAN"
              << "FORWARD"
              << "FOUND"
              << "FREE"
              << "FREEZE"
              << "FROM"
              << "FULL"
              << "FUNCTION"
              << "FUSION"
              << "G"
              << "GENERAL"
              << "GENERATED"
              << "GET"
              << "GLOBAL"
              << "GO"
              << "GOTO"
              << "GRANT"
              << "GRANTED"
              << "GREATEST"
              << "GROUP"
              << "GROUPING"
              << "HANDLER"
              << "HAVING"
              << "HEADER"
              << "HIERARCHY"
              << "HOLD"
              << "HOST"
              << "HOUR"
              << "IDENTITY"
              << "IF"
              << "IGNORE"
              << "ILIKE"
              << "IMMEDIATE"
              << "IMMUTABLE"
              << "IMPLEMENTATION"
              << "IMPLICIT"
              << "IN"
              << "INCLUDING"
              << "INCREMENT"
              << "INDEX"
              << "INDEXES"
              << "INDICATOR"
              << "INFIX"
              << "INHERIT"
              << "INHERITS"
              << "INITIALIZE"
              << "INITIALLY"
              << "INNER"
              << "INOUT"
              << "INPUT"
              << "INSENSITIVE"
              << "INSERT"
              << "INSTANCE"
              << "INSTANTIABLE"
              << "INSTEAD"
              << "INT"
              << "INTEGER"
              << "INTERSECT"
              << "INTERSECTION"
              << "INTERVAL"
              << "INTO"
              << "INVOKER"
              << "IS"
              << "ISNULL"
              << "ISOLATION"
              << "ITERATE"
              << "JOIN"
              << "K"
              << "KEY"
              << "KEY_MEMBER"
              << "KEY_TYPE"
              << "LANCOMPILER"
              << "LANGUAGE"
              << "LARGE"
              << "LAST"
              << "LATERAL"
              << "LEADING"
              << "LEAST"
              << "LEFT"
              << "LENGTH"
              << "LESS"
              << "LEVEL"
              << "LIKE"
              << "LIMIT"
              << "LISTEN"
              << "LN"
              << "LOAD"
              << "LOCAL"
              << "LOCALTIME"
              << "LOCALTIMESTAMP"
              << "LOCATION"
              << "LOCATOR"
              << "LOCK"
              << "LOGIN"
              << "LOWER"
              << "M"
              << "MAP"
              << "MATCH"
              << "MATCHED"
              << "MAX"
              << "MAXVALUE"
              << "MEMBER"
              << "MERGE"
              << "MESSAGE_LENGTH"
              << "MESSAGE_OCTET_LENGTH"
              << "MESSAGE_TEXT"
              << "METHOD"
              << "MIN"
              << "MINUTE"
              << "MINVALUE"
              << "MOD"
              << "MODE"
              << "MODIFIES"
              << "MODIFY"
              << "MODULE"
              << "MONTH"
              << "MORE"
              << "MOVE"
              << "MULTISET"
              << "MUMPS"
              << "NAME"
              << "NAMES"
              << "NATIONAL"
              << "NATURAL"
              << "NCHAR"
              << "NCLOB"
              << "NESTING"
              << "NEW"
              << "NEXT"
              << "NO"
              << "NOCREATEDB"
              << "NOCREATEROLE"
              << "NOCREATEUSER"
              << "NOINHERIT"
              << "NOLOGIN"
              << "NONE"
              << "NORMALIZE"
              << "NORMALIZED"
              << "NOSUPERUSER"
              << "NOT"
              << "NOTHING"
              << "NOTIFY"
              << "NOTNULL"
              << "NOWAIT"
              << "NULL"
              << "NULLABLE"
              << "NULLIF"
              << "NULLS"
              << "NUMBER"
              << "NUMERIC"
              << "OBJECT"
              << "OCTETS"
              << "OCTET_LENGTH"
              << "OF"
              << "OFF"
              << "OFFSET"
              << "OIDS"
              << "OLD"
              << "ON"
              << "ONLY"
              << "OPEN"
              << "OPERATION"
              << "OPERATOR"
              << "OPTION"
              << "OPTIONS"
              << "OR"
              << "ORDER"
              << "ORDERING"
              << "ORDINALITY"
              << "OTHERS"
              << "OUT"
              << "OUTER"
              << "OUTPUT"
              << "OVER"
              << "OVERLAPS"
              << "OVERLAY"
              << "OVERRIDING"
              << "OWNED"
              << "OWNER"
              << "PAD"
              << "PARAMETER"
              << "PARAMETERS"
              << "PARAMETER_MODE"
              << "PARAMETER_NAME"
              << "PARAMETER_ORDINAL_POSITION"
              << "PARAMETER_SPECIFIC_CATALOG"
              << "PARAMETER_SPECIFIC_NAME"
              << "PARAMETER_SPECIFIC_SCHEMA"
              << "PARTIAL"
              << "PARTITION"
              << "PASCAL"
              << "PASSWORD"
              << "PATH"
              << "PERCENTILE_CONT"
              << "PERCENTILE_DISC"
              << "PERCENT_RANK"
              << "PLACING"
              << "PLI"
              << "POSITION"
              << "POSTFIX"
              << "POWER"
              << "PRECEDING"
              << "PRECISION"
              << "PREFIX"
              << "PREORDER"
              << "PREPARE"
              << "PREPARED"
              << "PRESERVE"
              << "PRIMARY"
              << "PRIOR"
              << "PRIVILEGES"
              << "PROCEDURAL"
              << "PROCEDURE"
              << "PUBLIC"
              << "QUOTE"
              << "RANGE"
              << "RANK"
              << "READ"
              << "READS"
              << "REAL"
              << "REASSIGN"
              << "RECHECK"
              << "RECURSIVE"
              << "REF"
              << "REFERENCES"
              << "REFERENCING"
              << "REGR_AVGX"
              << "REGR_AVGY"
              << "REGR_COUNT"
              << "REGR_INTERCEPT"
              << "REGR_R2"
              << "REGR_SLOPE"
              << "REGR_SXX"
              << "REGR_SXY"
              << "REGR_SYY"
              << "REINDEX"
              << "RELATIVE"
              << "RELEASE"
              << "RENAME"
              << "REPEATABLE"
              << "REPLACE"
              << "RESET"
              << "RESTART"
              << "RESTRICT"
              << "RESULT"
              << "RETURN"
              << "RETURNED_CARDINALITY"
              << "RETURNED_LENGTH"
              << "RETURNED_OCTET_LENGTH"
              << "RETURNED_SQLSTATE"
              << "RETURNING"
              << "RETURNS"
              << "REVOKE"
              << "RIGHT"
              << "ROLE"
              << "ROLLBACK"
              << "ROLLUP"
              << "ROUTINE"
              << "ROUTINE_CATALOG"
              << "ROUTINE_NAME"
              << "ROUTINE_SCHEMA"
              << "ROW"
              << "ROWS"
              << "ROW_COUNT"
              << "ROW_NUMBER"
              << "RULE"
              << "SAVEPOINT"
              << "SCALE"
              << "SCHEMA"
              << "SCHEMA_NAME"
              << "SCOPE"
              << "SCOPE_CATALOG"
              << "SCOPE_NAME"
              << "SCOPE_SCHEMA"
              << "SCROLL"
              << "SEARCH"
              << "SECOND"
              << "SECTION"
              << "SECURITY"
              << "SELECT"
              << "SELF"
              << "SENSITIVE"
              << "SEQUENCE"
              << "SERIALIZABLE"
              << "SERVER_NAME"
              << "SESSION"
              << "SESSION_USER"
              << "SET"
              << "SETOF"
              << "SETS"
              << "SHARE"
              << "SHOW"
              << "SIMILAR"
              << "SIMPLE"
              << "SIZE"
              << "SMALLINT"
              << "SOME"
              << "SOURCE"
              << "SPACE"
              << "SPECIFIC"
              << "SPECIFICTYPE"
              << "SPECIFIC_NAME"
              << "SQL"
              << "SQLCODE"
              << "SQLERROR"
              << "SQLEXCEPTION"
              << "SQLSTATE"
              << "SQLWARNING"
              << "SQRT"
              << "STABLE"
              << "START"
              << "STATE"
              << "STATEMENT"
              << "STATIC"
              << "STATISTICS"
              << "STDDEV_POP"
              << "STDDEV_SAMP"
              << "STDIN"
              << "STDOUT"
              << "STORAGE"
              << "STRICT"
              << "STRUCTURE"
              << "STYLE"
              << "SUBCLASS_ORIGIN"
              << "SUBLIST"
              << "SUBMULTISET"
              << "SUBSTRING"
              << "SUM"
              << "SUPERUSER"
              << "SYMMETRIC"
              << "SYSID"
              << "SYSTEM"
              << "SYSTEM_USER"
              << "TABLE"
              << "TABLESAMPLE"
              << "TABLESPACE"
              << "TABLE_NAME"
              << "TEMP"
              << "TEMPLATE"
              << "TEMPORARY"
              << "TERMINATE"
              << "THAN"
              << "THEN"
              << "TIES"
              << "TIME"
              << "TIMESTAMP"
              << "TIMEZONE_HOUR"
              << "TIMEZONE_MINUTE"
              << "TO"
              << "TOP_LEVEL_COUNT"
              << "TRAILING"
              << "TRANSACTION"
              << "TRANSACTIONS_COMMITTED"
              << "TRANSACTIONS_ROLLED_BACK"
              << "TRANSACTION_ACTIVE"
              << "TRANSFORM"
              << "TRANSFORMS"
              << "TRANSLATE"
              << "TRANSLATION"
              << "TREAT"
              << "TRIGGER"
              << "TRIGGER_CATALOG"
              << "TRIGGER_NAME"
              << "TRIGGER_SCHEMA"
              << "TRIM"
              << "TRUE"
              << "TRUNCATE"
              << "TRUSTED"
              << "TYPE"
              << "UESCAPE"
              << "UNBOUNDED"
              << "UNCOMMITTED"
              << "UNDER"
              << "UNENCRYPTED"
              << "UNION"
              << "UNIQUE"
              << "UNKNOWN"
              << "UNLISTEN"
              << "UNNAMED"
              << "UNNEST"
              << "UNTIL"
              << "UPDATE"
              << "UPPER"
              << "USAGE"
              << "USER"
              << "USER_DEFINED_TYPE_CATALOG"
              << "USER_DEFINED_TYPE_CODE"
              << "USER_DEFINED_TYPE_NAME"
              << "USER_DEFINED_TYPE_SCHEMA"
              << "USING"
              << "VACUUM"
              << "VALID"
              << "VALIDATOR"
              << "VALUE"
              << "VALUES"
              << "VARCHAR"
              << "VARIABLE"
              << "VARYING"
              << "VAR_POP"
              << "VAR_SAMP"
              << "VERBOSE"
              << "VIEW"
              << "VOLATILE"
              << "WHEN"
              << "WHENEVER"
              << "WHERE"
              << "WIDTH_BUCKET"
              << "WINDOW"
              << "WITH"
              << "WITHIN"
              << "WITHOUT"
              << "WORK"
              << "WRITE"
              << "YEAR"
              << "ZONE"
              ;

  // these are metasql key words from http://www.xtuple.org/node/270
  if (_extension.isEmpty())
    _extension  << "break"
                << "continue"
                << "else"
                << "elseif"
                << "endforeach"
                << "endif"
                << "exists"
                << "foreach"
                << "if"
                << "isFirst"
                << "isLast"
                << "literal"
                << "not"
                << "reExists"
                << "value"
                ;

  _kwtest.setPattern("^(" + _keyword.join("|") + ")((?=\\W)|$)");
  _kwtest.setCaseSensitivity(Qt::CaseInsensitive);

  _extest.setPattern("^(" + _extension.join("|") + ")((?=\\W)|$)");
  _numerictest.setPattern("^(0[xX][0-9a-fA-F]+)|(-?[0-9]+(\\.[0-9]+)?)");
  _wordtest.setPattern("^\\w+");
  _sqlquotetest.setPattern("^'([^']|(''))+'");
  /*
  _metasqlquotetest.setPattern("^\"[^\"]+\"");
  */
}

MetaSQLHighlighter::MetaSQLHighlighter(QObject *parent)
  : QSyntaxHighlighter(parent)
{
  init();
}

MetaSQLHighlighter::MetaSQLHighlighter(QTextDocument *document)
  : QSyntaxHighlighter(document)
{
  init();
}

MetaSQLHighlighter::MetaSQLHighlighter(QTextEdit *editor)
  : QSyntaxHighlighter(editor)
{
  init();
}

MetaSQLHighlighter::~MetaSQLHighlighter()
{
}

void MetaSQLHighlighter::highlightBlock(const QString &text)
{
  int state = previousBlockState();
  int start = 0;

  for (int i = 0; i < text.length(); i++)
  {
    if (state == InsideCStyleComment)
    {
      if (text.mid(i, 2) == "*/")
      {
        state = NormalState;
        setFormat(start, i - start + 2, _commentColor);
      }
    }
    else if (state == InsideString && text.mid(i, 1) == "'" && text.mid(i, 2) != "''")
    {
      state = NormalState;
      setFormat(start, i - start + 1, _literalColor);
      start = i;
    }
    else if (state == InsideMetaSQL && _extest.indexIn(text.mid(i)) == 0)
    {
      setFormat(i, _extest.matchedLength(), _extensionColor);
      i += _extest.matchedLength();
      start = i;
    }
    else if (state == NormalState && text.mid(i, 2) == "<?")
    {
      state = InsideMetaSQL;
      setFormat(i, 2, _extensionColor);
      start = i + 1;
    }
    else if (state == InsideMetaSQL && text.mid(i, 2) == "?>")
    {
      state = NormalState;
      setFormat(i, 2, _extensionColor);
      start = i + 1;
    }
    else if (state == InsideMetaSQL && text.mid(i, 1) == "\"")
    {
      state = InsideMetaSQLString;
      setFormat(i, 1, _literalColor);
      i += 1;
    }
    else if (state == InsideMetaSQLString && text.mid(i, 1) == "\"")
    {
      state = InsideMetaSQL;
      setFormat(i, 1, _literalColor);
      i += 1;
    }
    else if (state == InsideMetaSQLString)
    {
      setFormat(i, 1, _literalColor);
      start = i;
    }
    else if (text.mid(i, 2) == "--")
    {
      setFormat(i, text.length() - i, _commentColor);
      break;
    }
    else if (state == NormalState && text.mid(i, 2) == "/*")
    {
      start = i;
      state = InsideCStyleComment;
    }
    else if (_sqlquotetest.indexIn(text.mid(i)) == 0)
    {
      setFormat(i, _sqlquotetest.matchedLength(), _literalColor);
      i += _sqlquotetest.matchedLength();
    }
    else if (text.mid(i, 2) == "''")
    {
      start = i;
      state = InsideString;
    }
    else if (_kwtest.indexIn(text.mid(i)) == 0)
    {
      setFormat(i, _kwtest.matchedLength(), _keywordColor);
      i += _kwtest.matchedLength();
    }
    else if (_numerictest.indexIn(text.mid(i)) == 0)
    {
      setFormat(i, _numerictest.matchedLength(), _literalColor);
      i += _numerictest.matchedLength();
    }
    else if (_wordtest.indexIn(text.mid(i)) == 0)        // skip non-keywords
      i += _wordtest.matchedLength();
  }

  if (state == InsideCStyleComment)
    setFormat(start, text.length() - start, _commentColor);
  else if (state == InsideString)
    setFormat(start, text.length() - start, _errorColor);

  setCurrentBlockState(state);
}
