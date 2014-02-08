/*
 * Copyright (c) 2012 - 2014, the Qtinuum project.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * <qtinuum.team@gmail.com>
 */

#include "PropertyDouble.h"

bool QtnPropertyDoubleBase::fromStrImpl(const QString& str)
{
    bool ok = false;
    ValueType value = str.toDouble(&ok);
    if (!ok)
        return false;

    return setValue(value);
}

bool QtnPropertyDoubleBase::toStrImpl(QString& str) const
{
    str = QString::number(value(), 'g', 10);
    return true;
}

bool QtnPropertyDoubleBase::fromVariantImpl(const QVariant& var)
{
    bool ok = false;
    ValueType value = var.toDouble(&ok);
    if (!ok)
        return false;

    return setValue(value);
}
