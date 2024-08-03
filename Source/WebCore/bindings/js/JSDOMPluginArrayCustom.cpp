/*
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSDOMPluginArray.h"

#include "DOMPluginArray.h"
#include "JSDOMBinding.h"
#include "JSDOMPlugin.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

using namespace JSC;

bool JSDOMPluginArray::nameGetter(ExecState* exec, PropertyName propertyName, JSValue& value)
{
    auto item = wrapped().namedItem(propertyNameToAtomicString(propertyName));
    if (!item)
        return false;

    value = toJS(exec, globalObject(), *item);
    return true;
}

} // namespace WebCore
