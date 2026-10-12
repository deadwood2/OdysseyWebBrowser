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
#include <fnmatch.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#if !OS(MORPHOS)
#include <sys/statvfs.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <wtf/EnumTraits.h>
#include <wtf/SafeStrerror.h>
#include <wtf/text/CString.h>
#include <wtf/text/MakeString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#include <wtf/HashMap.h>

#if OS(MORPHOS)
#include <libraries/charsets.h>
#include <proto/dos.h>
#include <proto/asyncio.h>
#endif

#if USE(GLIB)
#include <glib.h>
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
    case FileOpenMode::Truncate:
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

int posixFileDescriptor(PlatformFileHandle handle)
{
    return handle;
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

bool flushFile(PlatformFileHandle handle)
{
#if OS(MORPHOS)
    return true;
#else
    return !fsync(handle);
#endif
}

int64_t writeToFile(PlatformFileHandle handle, std::span<const uint8_t> data)
{
    do {
        auto bytesWritten = write(handle, data.data(), data.size());
        if (bytesWritten >= 0)
            return bytesWritten;
    } while (errno == EINTR);
    return -1;
}

int64_t readFromFile(PlatformFileHandle handle, std::span<uint8_t> data)
{
    do {
        auto bytesRead = read(handle, data.data(), data.size());
        if (bytesRead >= 0)
            return bytesRead;
    } while (errno == EINTR);
    return -1;
}

#if USE(FILE_LOCK)
bool lockFile(PlatformFileHandle handle, OptionSet<FileLockMode> lockMode)
{
    static_assert(LOCK_SH == WTF::enumToUnderlyingType(FileLockMode::Shared), "LockSharedEncoding is as expected");
    static_assert(LOCK_EX == WTF::enumToUnderlyingType(FileLockMode::Exclusive), "LockExclusiveEncoding is as expected");
    static_assert(LOCK_NB == WTF::enumToUnderlyingType(FileLockMode::Nonblocking), "LockNonblockingEncoding is as expected");
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
#if (OS(LINUX) && HAVE(STATX)) || OS(DARWIN) || OS(OPENBSD) || OS(NETBSD) || OS(FREEBSD)
    CString fsRep = fileSystemRepresentation(path);
    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return std::nullopt;

#if OS(LINUX) && HAVE(STATX)
    struct statx fileInfo;

    if (statx(-1, fsRep.data(), 0, STATX_BTIME, &fileInfo) == -1)
        return std::nullopt;

    return WallTime::fromRawSeconds(fileInfo.stx_btime.tv_sec);
#elif OS(DARWIN) || OS(OPENBSD) || OS(NETBSD) || OS(FREEBSD)
    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo) == -1)
        return std::nullopt;

    return WallTime::fromRawSeconds(fileInfo.st_birthtime);
#endif
#endif

    UNUSED_PARAM(path);
    return std::nullopt;
}

std::optional<PlatformFileID> fileID(PlatformFileHandle handle)
{
    struct stat fileInfo;
    if (fstat(handle, &fileInfo))
        return std::nullopt;

    return fileInfo.st_ino;
}

bool fileIDsAreEqual(std::optional<PlatformFileID> a, std::optional<PlatformFileID> b)
{
    return a == b;
}

std::optional<uint32_t> volumeFileBlockSize(const String& path)
{
#if !OS(MORPHOS)
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

#if OS(MORPHOS)
	return String(path, strlen(path), MIBENUM_SYSTEM);
#else
    return String::fromUTF8(path);
#endif
}

CString fileSystemRepresentation(const String& path)
{
#if OS(MORPHOS)
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
static const char* temporaryFileDirectory()
{
#if USE(GLIB)
    return g_get_tmp_dir();
#else
    if (auto* tmpDir = getenv("TMPDIR"))
        return tmpDir;

    return "/tmp";
#endif
}

std::pair<String, PlatformFileHandle> openTemporaryFile(StringView tmpPath, StringView prefix, StringView suffix)
{
    // Suffix is not supported because that's incompatible with mkstemp.
    // This is OK for now since the code using it is built on macOS only.
    ASSERT_UNUSED(suffix, suffix.isEmpty());

    PlatformFileHandle handle = invalidPlatformFileHandle;

    char buffer[PATH_MAX];
#if OS(MORPHOS)
	stccpy(buffer, fileSystemRepresentation(tmpPath.toString()).data(), sizeof(buffer));
	auto prefixadd = fileSystemRepresentation(prefix.toString());
	if (0 == AddPart(buffer, prefixadd.data(), sizeof(buffer)))
		goto end;
    if (strlen(buffer) >= PATH_MAX - 7)
    	goto end;
	strcat(buffer, "XXXXXX");
#else
    if (snprintf(buffer, PATH_MAX, "%s/%sXXXXXX", temporaryFileDirectory(), prefix.utf8().data()) >= PATH_MAX)
        goto end;
#endif
 
    handle = mkstemp(buffer);
    if (handle < 0)
        goto end;
 
#if OS(MORPHOS)
    return { String(buffer, strlen(buffer), MIBENUM_SYSTEM), handle };
#else
    return { String::fromUTF8(buffer), handle };
#endif
end:
    handle = invalidPlatformFileHandle;
    return { String(), handle };
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
#if OS(MORPHOS)
	tmpPathPrefixes.set(prefix, String(tmpPath, strlen(tmpPath), MIBENUM_SYSTEM));
#endif
}

std::pair<String, PlatformFileHandle> openTemporaryFile(StringView prefix, StringView suffix)
{
#if OS(MORPHOS)
	const char* tmpDir = "PROGDIR:Tmp";
    auto prefixStr = prefix.toString();
	if (tmpPathPrefixes.contains(prefixStr))
	{
		return openTemporaryFile(tmpPathPrefixes.get(prefixStr), prefix, suffix);
	}
	return openTemporaryFile(String(tmpDir, strlen(tmpDir), MIBENUM_SYSTEM), prefix, suffix);
#else
    const char* tmpDir = getenv("TMPDIR");

    if (!tmpDir)
        tmpDir = "/tmp";

	return openTemporaryFile(String::fromUTF8(tmpDir), prefix, handle);
#endif
}
#endif // !PLATFORM(COCOA)

std::optional<int32_t> getFileDeviceId(const String& path)
{
    auto fsFile = fileSystemRepresentation(path);
    if (fsFile.isNull())
        return std::nullopt;

    struct stat fileStat;
    if (stat(fsFile.data(), &fileStat) == -1)
        return std::nullopt;

    return fileStat.st_dev;
}

#if OS(MORPHOS)
static unsigned long tmpnum = 0;

int mkstempasync(char *path)
{
    char *str, *end = path;
    unsigned long num;
    int fd = -1;

    while (*end) end++;
    str = end;

    // this would need a lock in theory, but for the time being this is only being
    // called by curl on its own thread...
    num = tmpnum++;
    while (*--str == 'X')
    {
        *str = '0' + (num%10);
        num /= 10;
    }

    if (end > path && ++str < end)
    {
        while (fd == -1)
        {
            BPTR lock = Lock(path, SHARED_LOCK);
            if (lock == 0)
            {
                auto asyncfd = OpenAsync(path, MODE_WRITE, 512 * 1024);
                fd = int(asyncfd);
                if (0 == fd)
                    fd = -1;
            }
            else
            {
                UnLock(lock);
                char *s;

                // see above
                num = tmpnum++;

                s = end;
                while (s-- > str)
                {
                    *s = '0' + (num%10);
                    num /= 10;
                }
            }
        }
    }

    return fd;
}

std::pair<String, PlatformFileHandle> openTemporaryFileAsync(StringView prefix)
{
    char buffer[PATH_MAX];

    PlatformFileHandle handle = invalidPlatformFileHandle;

	const char* tmpDirIn = "PROGDIR:Tmp";
    String tmpDir = String(tmpDirIn, strlen(tmpDirIn), MIBENUM_SYSTEM);
    auto prefixStr = prefix.toString();
	if (tmpPathPrefixes.contains(prefixStr))
        tmpDir = tmpPathPrefixes.get(prefixStr);

	stccpy(buffer, fileSystemRepresentation(tmpDir).data(), sizeof(buffer));
	auto prefixadd = fileSystemRepresentation(prefix.toString());
	if (0 == AddPart(buffer, prefixadd.data(), sizeof(buffer)))
		goto end;
    if (strlen(buffer) >= PATH_MAX - 7)
    	goto end;
	strcat(buffer, "XXXXXX");

    handle = mkstempasync(buffer);
    if (handle == -1)
        goto end;

	return { String(buffer, strlen(buffer), MIBENUM_SYSTEM), handle };
end:
    handle = invalidPlatformFileHandle;
    return { String(), handle };
}

PlatformFileHandle openFileAsync(const String& path, FileOpenMode mode, FileAccessPermission, bool failIfFileExists)
{
    CString fsRep = fileSystemRepresentation(path);

    if (fsRep.isNull())
        return invalidPlatformFileHandle;

    OpenModes dosMode = MODE_READ;
    switch (mode) {
    case FileOpenMode::Read:
        break;
    case FileOpenMode::Truncate:
        dosMode = MODE_WRITE;
        break;
    case FileOpenMode::ReadWrite:
        dosMode = MODE_APPEND;
        break;
    }

    // not handled but we currently do not need this to work!
    (void)failIfFileExists;

    PlatformFileHandle fh = PlatformFileHandle(OpenAsync(STRPTR(fsRep.data()), dosMode, 512 * 1024));

    if (0 == fh)
        return -1;
    return fh;
}

void closeFileAsync(PlatformFileHandle& fh)
{
    if (fh != -1)
        CloseAsync((AsyncFile *)fh);
    fh = -1;
}

long long seekFileAsync(PlatformFileHandle fh, long long offset, FileSeekOrigin origin)
{
    SeekModes whence = MODE_CURRENT;
    switch (origin) {
    case FileSeekOrigin::Current:
        break;
    case FileSeekOrigin::End:
        whence = MODE_END;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (fh != -1)
        return SeekAsync64((AsyncFile *)fh, offset, whence);
    return -1;
}

int writeToFileAsync(PlatformFileHandle fh, std::span<const uint8_t> data)
{
    if (fh != -1)
        return WriteAsync((AsyncFile *)fh, APTR(data.data()), ULONG(data.size()));
    return -1;
}
#endif

// On macOS, stat() used by std::filesystem is much slower than access() when sandboxed.
// This fast path exists to avoid calls to stat(). It's not needed on other platforms.
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
        LOG_ERROR("File failed to delete. Error message: %s", safeStrerror(errno).data());

    return unlinked;
}

bool makeAllDirectories(const String& path)
{
    auto fullPath = fileSystemRepresentation(path);
    int length = fullPath.length();
    if (!length)
        return false;

    if (!access(fullPath.data(), F_OK))
        return true;

    char* p = fullPath.mutableData() + 1;
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

String pathByAppendingComponent(StringView path, StringView component)
{
#if OS(MORPHOS)
    if (path.endsWith('/') || path.endsWith(':'))
#else
    if (path.endsWith('/'))
#endif
        return makeString(path, component);
    return makeString(path, '/', component);
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
