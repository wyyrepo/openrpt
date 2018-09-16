#!/bin/bash

###################################
# OpenRPT report writer and rendering engine
# Copyright (C) 2001-2014 by OpenMFG, LLC
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# Please contact info@openmfg.com with any questions on this license.
###################################
#
# Chris Newey 15 June 2007
# Email newey499 at hotmail.com
#
###################################
#
# Test harness for bldtbls utility
#
# Note: when using odbc need three slashes to get the databaseURL parsed correctly.
#       ODBC connections don't know anything about a server IP from the application side
# Eg. -databaseURL=odbc:///enigma
#     as opposed to 
#     -databaseURL=psql://127.0.0.1/enigma
#
# 
###################################


clear
echo "Test Start"

# no args
#./bldtbls

# bad url
#./bldtbls -databaseURL=garbage -username=cdn -passwd=charlton -dbengine=mysql

# bad user
#./bldtbls -databaseURL=odbc:///enigma -username=garbage -passwd=charlton -dbengine=mysql

# bad passwd
#./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=garbage -dbengine=mysql

# breaks - MS Access - never heard of it
./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton -dbengine=access

# breaks - SQL Server - never heard of it
./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton -dbengine=sqlserver

# Should run
#./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton -dbengine=mysql

# Breaks - odbc connection to mysql database but postgresql dbengine passed
#          results in an SQL syntax error from the MySQL database 
#./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton -dbengine=psql

# breaks - non existent ODBC connection
#./bldtbls -databaseURL=odbc:///garbage -username=cdn -passwd=charlton -dbengine=mysql

# bad -dbengine
#./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton -dbengine=QWERTY

# breaks - -dbengine reqd when protocol is odbc
#./bldtbls -databaseURL=odbc:///enigma -username=cdn -passwd=charlton 

# mysql - bad -dbengine
#./bldtbls -databaseURL=mysql://127.0.0.1/enigma -username=cdn -passwd=charlton -dbengine=QWERTY

# postgres - bad dbengine
#./bldtbls -databaseURL=pgsql://127.0.0.1/enigma -username=cdn -passwd=charlton -dbengine=QWERTY

# should work
#./bldtbls -databaseURL=psql://127.0.0.1/enigma -username=cdn -passwd=charlton 

# breaks - bad  db2 not supported also bad dbengine
#./bldtbls -databaseURL=db2://127.0.0.1/enigma -username=cdn -passwd=charlton -dbengine=QWERTY

# breaks  - db2 not supported
#./bldtbls -databaseURL=db2://127.0.0.1/enigma -username=cdn -passwd=charlton 

# breaks bad url
#./bldtbls -databaseURL=garbage://127.0.0.1/enigma -username=cdn -passwd=charlton 

echo "Test Complete"
