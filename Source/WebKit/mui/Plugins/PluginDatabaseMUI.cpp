/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginDatabase.h"

#include "Frame.h"
#include "URL.h"
#include "PluginPackage.h"
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
#include "FileSystem.h"
#endif
#include <stdlib.h>
#include <wtf/text/CString.h>

namespace WebCore {

Vector<String> PluginDatabase::defaultPluginDirectories()
{
    Vector<String> paths;
#if !PLATFORM(MUI)
    String userPluginPath = homeDirectoryPath();
    userPluginPath.append(String("\\Application Data\\Mozilla\\plugins"));
    paths.append(userPluginPath);
#endif

    return paths;
}

bool PluginDatabase::isPreferredPluginDirectory(const String& path)
{
#if !PLATFORM(MUI)
    String preferredPath = homeDirectoryPath();

    preferredPath.append(String("\\Application Data\\Mozilla\\plugins"));

    // TODO: We should normalize the path before doing a comparison.
    return path == preferredPath;
#else
    return true;
#endif
}

void PluginDatabase::getPluginPathsInDirectories(HashSet<String>& paths) const
{
    // FIXME: This should be a case insensitive set.
    HashSet<String> uniqueFilenames;

    String fileNameFilter("");

    Vector<String>::const_iterator dirsEnd = m_pluginDirectories.end();
    for (Vector<String>::const_iterator dIt = m_pluginDirectories.begin(); dIt != dirsEnd; ++dIt) {
        Vector<String> pluginPaths = listDirectory(*dIt, fileNameFilter);
        Vector<String>::const_iterator pluginsEnd = pluginPaths.end();
        for (Vector<String>::const_iterator pIt = pluginPaths.begin(); pIt != pluginsEnd; ++pIt) {
            if (!fileExistsAndIsNotDisabled(*pIt))
                continue;

            paths.add(*pIt);
        }
    }
}

}
