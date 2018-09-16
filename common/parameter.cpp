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

#include "parameter.h"
#include <QtDebug>

Parameter::Parameter()
{
}

Parameter::Parameter(const QString &pName, const QVariant &pValue)
{
  _name  = pName;
  _value = pValue;
}

Parameter::Parameter(const Parameter &pParam)
{
  _name  = pParam._name;
  _value = pParam._value;
}

Parameter & Parameter::operator= (const Parameter &pParam)
{
  _name  = pParam._name;
  _value = pParam._value;

  return *this;
}

void ParameterList::append(const QString &pName)
{
  append(Parameter(pName, QVariant()));
}

void ParameterList::append(const QString &pName, const QVariant &pValue)
{
  append(Parameter(pName, pValue));
}

void ParameterList::append(const QString &pName, const QString &pValue)
{
  append(Parameter(pName, pValue));
}

void ParameterList::append(const QString &pName, const char *pValue)
{
  append(Parameter(pName, QString(pValue)));
}

void ParameterList::append(const QString &pName, const QDate &pValue)
{
  append(Parameter(pName, QVariant(pValue)));
}

void ParameterList::append(const QString &pName, unsigned int pValue)
{
  append(Parameter(pName, pValue));
}

void ParameterList::append(const QString &pName, int pValue)
{
  append(Parameter(pName, pValue));
}

void ParameterList::append(const QString &pName, double pValue)
{
  append(Parameter(pName, pValue));
}

void ParameterList::append(const Parameter &pParam)
{
  QList<Parameter>::append(pParam);
}

void ParameterList::remove( QString &pParamName)
{
	int remove = -1;
	for (int counter = 0; counter < count(); counter++)
	{
		if (operator[](counter)._name == pParamName)
		{
			remove = counter;
		}
	}

	if (remove > 0)
	{    
		removeAt(remove);
	}
}

QString ParameterList::name(int pAt) const
{
  return operator[](pAt)._name;
}

QVariant ParameterList::value(int pAt, bool *pValid) const
{
  if (pValid != NULL)
    *pValid = true;

  return operator[](pAt)._value;
}

QVariant ParameterList::value(const QString &pParamName, bool *pValid) const
{
  for (int counter = 0; counter < count(); counter++)
  {
    if (operator[](counter)._name == pParamName)
    {
      if (pValid != NULL)
        *pValid = true;

      return operator[](counter)._value;
    }
  }

  if (pValid != NULL)
    *pValid = false;

  return QVariant();
}

bool ParameterList::inList(const QString &pParamName) const
{
  for (int counter = 0; counter < count(); counter++)
  {
    if (operator[](counter)._name == pParamName)
      return true;
  }

  return false;
}

