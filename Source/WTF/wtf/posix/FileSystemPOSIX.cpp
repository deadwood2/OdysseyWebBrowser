/*
 * Copyright (C) 2007-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/FileSystem.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#if !OS(AROS)
#include <fnmatch.h>
#endif
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#if !PLATFORM(MUI)
#include <sys/statvfs.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <wtf/EnumTraits.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#include <wtf/HashMap.h>

#if PLATFORM(MUI)
#if OS(MORPHOS)
#include <libraries/charsets.h>
#endif
#include <proto/dos.h>
#endif

namespace WTF {

namespace FileSystemImpl {

PlatformFileHandle openFile(const String& path, FileOpenMode mode, FileAccessPermission permission, bool failIfFileExists)
{
    CString fsRep = fileSystemRepresentation(path);

    if (fsRep.isNull())
        return invalidPlatformFileHandle;

    int platformFlag = 0;
    switch (mode) {
    case FileOpenMode::Read:
        platformFlag |= O_RDONLY;
        break;
    case FileOpenMode::Write:
        platformFlag |= (O_WRONLY | O_CREAT | O_TRUNC);
        break;
    case FileOpenMode::ReadWrite:
        platformFlag |= (O_RDWR | O_CREAT);
        break;
#if OS(DARWIN)
    case FileOpenMode::EventsOnly:
        platformFlag |= O_EVTONLY;
        break;
#endif
    }

    if (failIfFileExists)
        platformFlag |= (O_CREAT | O_EXCL);

    int permissionFlag = 0;
    if (permission == FileAccessPermission::User)
        permissionFlag |= (S_IRUSR | S_IWUSR);
    else if (permission == FileAccessPermission::All)
        permissionFlag |= (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    return open(fsRep.data(), platformFlag, permissionFlag);
}

void closeFile(PlatformFileHandle& handle)
{
    if (isHandleValid(handle)) {
        close(handle);
        handle = invalidPlatformFileHandle;
    }
}

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    int whence = SEEK_SET;
    switch (origin) {
    case FileSeekOrigin::Beginning:
        whence = SEEK_SET;
        break;
    case FileSeekOrigin::Current:
        whence = SEEK_CUR;
        break;
    case FileSeekOrigin::End:
        whence = SEEK_END;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    return static_cast<long long>(lseek(handle, offset, whence));
}

bool truncateFile(PlatformFileHandle handle, long long offset)
{
    // ftruncate returns 0 to indicate the success.
    return !ftruncate(handle, offset);
}

int writeToFile(PlatformFileHandle handle, const void* data, int length)
{
    do {
        int bytesWritten = write(handle, data, static_cast<size_t>(length));
        if (bytesWritten >= 0)
            return bytesWritten;
    } while (errno == EINTR);
    return -1;
}

int readFromFile(PlatformFileHandle handle, void* data, int length)
{
    do {
        int bytesRead = read(handle, data, static_cast<size_t>(length));
        if (bytesRead >= 0)
            return bytesRead;
    } while (errno == EINTR);
    return -1;
}

#if USE(FILE_LOCK)
bool lockFile(PlatformFileHandle handle, OptionSet<FileLockMode> lockMode)
{
    COMPILE_ASSERT(LOCK_SH == WTF::enumToUnderlyingType(FileLockMode::Shared), LockSharedEncodingIsAsExpected);
    COMPILE_ASSERT(LOCK_EX == WTF::enumToUnderlyingType(FileLockMode::Exclusive), LockExclusiveEncodingIsAsExpected);
    COMPILE_ASSERT(LOCK_NB == WTF::enumToUnderlyingType(FileLockMode::Nonblocking), LockNonblockingEncodingIsAsExpected);
    int result = flock(handle, lockMode.toRaw());
    return (result != -1);
}

bool unlockFile(PlatformFileHandle handle)
{
    int result = flock(handle, LOCK_UN);
    return (result != -1);
}
#endif

std::optional<uint64_t> fileSize(PlatformFileHandle handle)
{
    struct stat fileInfo;
    if (fstat(handle, &fileInfo))
        return std::nullopt;

    return fileInfo.st_size;
}

std::optional<WallTime> fileCreationTime(const String& path)
{
#if OS(DARWIN) || OS(OPENBSD) || OS(NETBSD) || OS(FREEBSD)
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return std::nullopt;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return std::nullopt;

    return WallTime::fromRawSeconds(fileInfo.st_birthtime);
#else
    UNUSED_PARAM(path);
    return std::nullopt;
#endif
}

std::optional<uint32_t> volumeFileBlockSize(const String& path)
{
#if !PLATFORM(MUI)
    struct statvfs fileStat;
    if (!statvfs(fileSystemRepresentation(path).data(), &fileStat))
        return fileStat.f_frsize;
#endif
    return std::nullopt;
}

#if !USE(CF)
String stringFromFileSystemRepresentation(const char* path)
{
    if (!path)
        return String();
#if PLATFORM(MUI)
#if OS(MORPHOS)
	return String(path, strlen(path), MIBENUM_SYSTEM);
#endif
#if OS(AROS)
	return String(path, strlen(path));
#endif
#else
    return String::fromUTF8(path);
#endif
}

CString fileSystemRepresentation(const String& path)
{
#if PLATFORM(MUI)
	// some fixes for unix style path fuckups here...
	// file:///progdir:foo will give us /progdir:foo, so let's account for that
	if (path.contains(':') && path.startsWith('/'))
	{
		String sub = path.substring(1);
		return sub.native();
	}
	return path.native();
#else
    return path.utf8();
#endif
}
#endif

#if !PLATFORM(COCOA)
String openTemporaryFile(const String& tmpPath, const String& prefix, PlatformFileHandle& handle, const String& suffix)
{
    // FIXME: Suffix is not supported, but OK for now since the code using it is macOS-port-only.
    ASSERT_UNUSED(suffix, suffix.isEmpty());

    char buffer[PATH_MAX];
#if PLATFORM(MUI)
	stccpy(buffer, fileSystemRepresentation(tmpPath).data(), sizeof(buffer));
	auto prefixadd = fileSystemRepresentation(prefix);
	if (0 == AddPart(buffer, prefixadd.data(), sizeof(buffer)))
		goto end;
    if (strlen(buffer) >= PATH_MAX - 7)
    	goto end;
	strcat(buffer, "XXXXXX");
#else
    const char* tmpDir = getenv("TMPDIR");

    if (!tmpDir)
        tmpDir = "/tmp";

    if (snprintf(buffer, PATH_MAX, "%s/%sXXXXXX", tmpDir, prefix.utf8().data()) >= PATH_MAX)
        goto end;
#endif

    handle = mkstemp(buffer);
    if (handle < 0)
        goto end;

#if PLATFORM(MUI)
#if OS(MORPHOS)
	return String(buffer, strlen(buffer), MIBENUM_SYSTEM);
#endif
#if OS(AROS)
	return String(buffer, strlen(buffer));
#endif
#else
    return String::fromUTF8(buffer);
#endif

end:
    handle = invalidPlatformFileHandle;
    return String();
}

HashMap<String, String> tmpPathPrefixes;

String temporaryFilePathForPrefix(const String& prefix)
{
	if (tmpPathPrefixes.contains(prefix))
		return tmpPathPrefixes.get(prefix);
	return { };
}

void setTemporaryFilePathForPrefix(const char * tmpPath, const String& prefix)
{
#if PLATFORM(MUI)
#if OS(MORPHOS)
	tmpPathPrefixes.set(prefix, String(tmpPath, strlen(tmpPath), MIBENUM_SYSTEM));
#endif
#if OS(AROS)
	tmpPathPrefixes.set(prefix, String(tmpPath, strlen(tmpPath)));
#endif
#endif
}

String openTemporaryFile(const String& prefix, PlatformFileHandle& handle, const String& suffix)
{
#if PLATFORM(MUI)
	const char* tmpDir = "PROGDIR:Tmp";
	if (tmpPathPrefixes.contains(prefix))
	{
		return openTemporaryFile(tmpPathPrefixes.get(prefix), prefix, handle, suffix);
	}
#if OS(MORPHOS)
	return openTemporaryFile(String(tmpDir, strlen(tmpDir), MIBENUM_SYSTEM), prefix, handle, suffix);
#endif
#if OS(AROS)
	return openTemporaryFile(String(tmpDir, strlen(tmpDir)), prefix, handle, suffix);
#endif
#else
    const char* tmpDir = getenv("TMPDIR");

    if (!tmpDir)
        tmpDir = "/tmp";

	return openTemporaryFile(String::fromUTF8(tmpDir), prefix, handle);
#endif
}
#endif // !PLATFORM(COCOA)

std::optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    struct stat fileStat;
    if (stat(fsFile.data(), &fileStat) == -1)
        return std::nullopt;

    return fileStat.st_dev;
}

#if ENABLE(FILESYSTEM_POSIX_FAST_PATH)

bool fileExists(const String& path)
{
    return access(fileSystemRepresentation(path).data(), F_OK) != -1;
}

bool deleteFile(const String& path)
{
    // unlink(...) returns 0 on successful deletion of the path and non-zero in any other case (including invalid permissions or non-existent file)
    bool unlinked = !unlink(fileSystemRepresentation(path).data());
    if (!unlinked && errno != ENOENT)
        LOG_ERROR("File failed to delete. Error message: %s", strerror(errno));

    return unlinked;
}

bool makeAllDirectories(const String& path)
{
    auto fullPath = fileSystemRepresentation(path);
    if (!access(fullPath.data(), F_OK))
        return true;

    char* p = fullPath.mutableData() + 1;
    int length = fullPath.length();
    if (p[length - 1] == '/')
        p[length - 1] = '\0';
    for (; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (access(fullPath.data(), F_OK)) {
                if (mkdir(fullPath.data(), S_IRWXU))
                    return false;
            }
            *p = '/';
        }
    }
    if (access(fullPath.data(), F_OK)) {
        if (mkdir(fullPath.data(), S_IRWXU))
            return false;
    }

    return true;
}

String pathByAppendingComponent(const String& path, const String& component)
{
#if PLATFORM(MUI)
    if (path.endsWith('/') || path.endsWith(':'))
#else
      if (path.endsWith('/'))
#endif
        return path + component;
    return path + "/" + component;
}

String pathByAppendingComponents(StringView path, const Vector<StringView>& components)
{
    StringBuilder builder;
    builder.append(path);
    bool isFirstComponent = true;
    for (auto& component : components) {
        if (isFirstComponent) {
            isFirstComponent = false;
            if (path.endsWith('/')) {
                builder.append(component);
                continue;
            }
        }
        builder.append('/', component);
    }
    return builder.toString();
}

#endif

} // namespace FileSystemImpl
} // namespace WTF
