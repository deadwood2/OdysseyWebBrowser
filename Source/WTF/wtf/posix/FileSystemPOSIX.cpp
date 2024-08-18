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
#if !PLATFORM(MUI)
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
#include <wtf/FileMetadata.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#include <wtf/HashMap.h>

#if PLATFORM(MUI)
#if OS(MORPHOS)
#include <libraries/charsets.h>
#endif
#if OS(AROS)
#define native latin1
#endif
#include <proto/dos.h>
#endif

namespace WTF {

namespace FileSystemImpl {

bool fileExists(const String& path)
{
    if (path.isNull())
        return false;

    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    return access(fsRep.data(), F_OK) != -1;
}

bool deleteFile(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0') {
        LOG_ERROR("File failed to delete. Failed to get filesystem representation to create CString from cfString or filesystem representation is a null value");
        return false;
    }

    // unlink(...) returns 0 on successful deletion of the path and non-zero in any other case (including invalid permissions or non-existent file)
    bool unlinked = !unlink(fsRep.data());
    if (!unlinked && errno != ENOENT)
        LOG_ERROR("File failed to delete. Error message: %s", strerror(errno));

    return unlinked;
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
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
//    case FileOpenMode::ReadWrite:
//        platformFlag |= (O_RDWR | O_CREAT);
//        break;
#if OS(DARWIN)
    case FileOpenMode::EventsOnly:
        platformFlag |= O_EVTONLY;
        break;
#endif
    }

//    if (failIfFileExists)
//        platformFlag |= (O_CREAT | O_EXCL);

    int permissionFlag = 0;
//    if (permission == FileAccessPermission::User)
//        permissionFlag |= (S_IRUSR | S_IWUSR);
//    else if (permission == FileAccessPermission::All)
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

int writeToFile(PlatformFileHandle handle, const char* data, int length)
{
    do {
        int bytesWritten = write(handle, data, static_cast<size_t>(length));
        if (bytesWritten >= 0)
            return bytesWritten;
    } while (errno == EINTR);
    return -1;
}

int readFromFile(PlatformFileHandle handle, char* data, int length)
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

#if !PLATFORM(MAC)
bool deleteEmptyDirectory(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    // rmdir(...) returns 0 on successful deletion of the path and non-zero in any other case (including invalid permissions or non-existent file)
    return !rmdir(fsRep.data());
}
#endif

bool getFileSize(const String& path, long long& result)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return false;

    result = fileInfo.st_size;
    return true;
}

bool getFileSize(PlatformFileHandle handle, long long& result)
{
    struct stat fileInfo;
    if (fstat(handle, &fileInfo))
        return false;

    result = fileInfo.st_size;
    return true;
}

Optional<WallTime> getFileCreationTime(const String& path)
{
#if OS(DARWIN) || OS(OPENBSD) || OS(NETBSD) || OS(FREEBSD)
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return WTF::nullopt;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return WTF::nullopt;

    return WallTime::fromRawSeconds(fileInfo.st_birthtime);
#else
    UNUSED_PARAM(path);
    return WTF::nullopt;
#endif
}

Optional<WallTime> getFileModificationTime(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return WTF::nullopt;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return WTF::nullopt;

    return WallTime::fromRawSeconds(fileInfo.st_mtime);
}

static FileMetadata::Type toFileMetataType(struct stat fileInfo)
{
    if (S_ISDIR(fileInfo.st_mode))
        return FileMetadata::Type::Directory;
    if (S_ISLNK(fileInfo.st_mode))
        return FileMetadata::Type::SymbolicLink;
    return FileMetadata::Type::File;
}

static Optional<FileMetadata> fileMetadataUsingFunction(const String& path, int (*statFunc)(const char*, struct stat*))
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return WTF::nullopt;

    struct stat fileInfo;
    if (statFunc(fsRep.data(), &fileInfo))
        return WTF::nullopt;

    String filename = pathGetFileName(path);
    bool isHidden = !filename.isEmpty() && filename[0] == '.';
    return FileMetadata {
        WallTime::fromRawSeconds(fileInfo.st_mtime),
        fileInfo.st_size,
        isHidden,
        toFileMetataType(fileInfo)
    };
}

Optional<FileMetadata> fileMetadata(const String& path)
{
    return fileMetadataUsingFunction(path, &lstat);
}

Optional<FileMetadata> fileMetadataFollowingSymlinks(const String& path)
{
    return fileMetadataUsingFunction(path, &stat);
}

bool createSymbolicLink(const String& targetPath, const String& symbolicLinkPath)
{
#if PLATFORM(MUI)
    return false;
#else
    CString targetPathFSRep = fileSystemRepresentation(targetPath);
    if (!targetPathFSRep.data() || targetPathFSRep.data()[0] == '\0')
        return false;

    CString symbolicLinkPathFSRep = fileSystemRepresentation(symbolicLinkPath);
    if (!symbolicLinkPathFSRep.data() || symbolicLinkPathFSRep.data()[0] == '\0')
        return false;

    return !symlink(targetPathFSRep.data(), symbolicLinkPathFSRep.data());
#endif
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
    for (auto& component : components) {
        builder.append('/');
        builder.append(component);
    }
    return builder.toString();
}

bool makeAllDirectories(const String& path)
{
    CString fullPath = fileSystemRepresentation(path);
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

String pathGetFileName(const String& path)
{
#if PLATFORM(MUI)
    auto position = path.reverseFind('/');
    if (position == notFound) {
        position = path.reverseFind(':');
    }
    return path.substring(position + 1);
#else
    return path.substring(path.reverseFind('/') + 1);
#endif
}

String directoryName(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return String();

    return String::fromUTF8(dirname(fsRep.mutableData()));
}

#if OS(AROS)
/* Copied from:
https://github.com/freebsd/freebsd-src/blob/main/sys/libkern/fnmatch.c
SPDX-License-Identifier: BSD-3-Clause
*/
#define	EOS	'\0'

#define RANGE_MATCH     1
#define RANGE_NOMATCH   0
#define RANGE_ERROR     (-1)

#define	FNM_NOMATCH	1	/* Match failed. */

#define	FNM_NOESCAPE	0x01	/* Disable backslash escaping. */
#define	FNM_PATHNAME	0x02	/* Slash must be matched by slash. */
#define	FNM_PERIOD	0x04	/* Period must be matched by period. */

#define	FNM_LEADING_DIR	0x08	/* Ignore /<tail> after Imatch. */
#define	FNM_CASEFOLD	0x10	/* Case insensitive search. */
#define	FNM_IGNORECASE	FNM_CASEFOLD
#define	FNM_FILE_NAME	FNM_PATHNAME

int
fnmatch(const char *pattern, const char *string, int flags)
{
	const char *stringstart;
//	char *newp;
	char c, test;

	for (stringstart = string;;)
		switch (c = *pattern++) {
		case EOS:
			if ((flags & FNM_LEADING_DIR) && *string == '/')
				return (0);
			return (*string == EOS ? 0 : FNM_NOMATCH);
		case '?':
			if (*string == EOS)
				return (FNM_NOMATCH);
			if (*string == '/' && (flags & FNM_PATHNAME))
				return (FNM_NOMATCH);
			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				return (FNM_NOMATCH);
			++string;
			break;
		case '*':
			c = *pattern;
			/* Collapse multiple stars. */
			while (c == '*')
				c = *++pattern;

			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				return (FNM_NOMATCH);

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS)
				if (flags & FNM_PATHNAME)
					return ((flags & FNM_LEADING_DIR) ||
					    strchr(string, '/') == NULL ?
					    0 : FNM_NOMATCH);
				else
					return (0);
			else if (c == '/' && flags & FNM_PATHNAME) {
				if ((string = strchr(string, '/')) == NULL)
					return (FNM_NOMATCH);
				break;
			}

			/* General case, use recursion. */
			while ((test = *string) != EOS) {
				if (!fnmatch(pattern, string, flags & ~FNM_PERIOD))
					return (0);
				if (test == '/' && flags & FNM_PATHNAME)
					break;
				++string;
			}
			return (FNM_NOMATCH);
/*		case '[':
			if (*string == EOS)
				return (FNM_NOMATCH);
			if (*string == '/' && (flags & FNM_PATHNAME))
				return (FNM_NOMATCH);
			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				return (FNM_NOMATCH);

			switch (rangematch(pattern, *string, flags, &newp)) {
			case RANGE_ERROR:
				goto norm;
			case RANGE_MATCH:
				pattern = newp;
				break;
			case RANGE_NOMATCH:
				return (FNM_NOMATCH);
			}
			++string;
			break;*/
		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				if ((c = *pattern++) == EOS) {
					c = '\\';
					--pattern;
				}
			}
			/* FALLTHROUGH */
		default:
//		norm:
			if (c == *string)
				;
			else if ((flags & FNM_CASEFOLD) &&
				 (tolower((unsigned char)c) ==
				  tolower((unsigned char)*string)))
				;
			else
				return (FNM_NOMATCH);
			string++;
			break;
		}
	/* NOTREACHED */
}
#endif
Vector<String> listDirectory(const String& path, const String& filter)
{
    Vector<String> entries;
    CString cpath = fileSystemRepresentation(path);
    CString cfilter = fileSystemRepresentation(filter);
    DIR* dir = opendir(cpath.data());
    if (dir) {
        struct dirent* dp;
        while ((dp = readdir(dir))) {
            const char* name = dp->d_name;
            if (!strcmp(name, ".") || !strcmp(name, ".."))
                continue;
            if (fnmatch(cfilter.data(), name, 0))
                continue;
            char filePath[PATH_MAX];
            if (static_cast<int>(sizeof(filePath) - 1) < snprintf(filePath, sizeof(filePath), "%s/%s", cpath.data(), name))
                continue; // buffer overflow

            auto string = stringFromFileSystemRepresentation(filePath);

            // Some file system representations cannot be represented as a UTF-16 string,
            // so this string might be null.
            if (!string.isNull())
                entries.append(WTFMove(string));
        }
        closedir(dir);
    }
    return entries;
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
bool moveFile(const String& oldPath, const String& newPath)
{
    auto oldFilename = fileSystemRepresentation(oldPath);
    if (oldFilename.isNull())
        return false;

    auto newFilename = fileSystemRepresentation(newPath);
    if (newFilename.isNull())
        return false;

    return rename(oldFilename.data(), newFilename.data()) != -1;
}

bool getVolumeFreeSpace(const String& path, uint64_t& freeSpace)
{
#if PLATFORM(MUI)
	return false;
#else
    struct statvfs fileSystemStat;
    if (statvfs(fileSystemRepresentation(path).data(), &fileSystemStat)) {
        freeSpace = fileSystemStat.f_bavail * fileSystemStat.f_frsize;
        return true;
    }
    return false;
#endif
}

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
    if (snprintf(buffer, PATH_MAX, "%s/%sXXXXXX", fileSystemRepresentation(tmpPath).data(), fileSystemRepresentation(prefix).data()) >= PATH_MAX)
        goto end;
#endif

    handle = mkstemp(buffer);
    if (handle < 0)
        goto end;

    return String::fromUTF8(buffer);

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
#if OS(MORPHOS)
	tmpPathPrefixes.set(prefix, String(tmpPath, strlen(tmpPath), MIBENUM_SYSTEM));
#endif
#if OS(AROS)
	tmpPathPrefixes.set(prefix, String(tmpPath, strlen(tmpPath)));
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

bool hardLink(const String& source, const String& destination)
{
#if PLATFORM(MUI)
    return false;
#else
    if (source.isEmpty() || destination.isEmpty())
        return false;

    auto fsSource = fileSystemRepresentation(source);
    if (!fsSource.data())
        return false;

    auto fsDestination = fileSystemRepresentation(destination);
    if (!fsDestination.data())
        return false;

    return !link(fsSource.data(), fsDestination.data());
#endif
}

bool hardLinkOrCopyFile(const String& source, const String& destination)
{
    if (hardLink(source, destination))
        return true;

    // Hard link failed. Perform a copy instead.
    if (source.isEmpty() || destination.isEmpty())
        return false;

    auto fsSource = fileSystemRepresentation(source);
    if (!fsSource.data())
        return false;

    auto fsDestination = fileSystemRepresentation(destination);
    if (!fsDestination.data())
        return false;

    auto handle = open(fsDestination.data(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (handle == -1)
        return false;

    bool appendResult = appendFileContentsToFileHandle(source, handle);
    close(handle);

    // If the copy failed, delete the unusable file.
    if (!appendResult)
        unlink(fsDestination.data());

    return appendResult;
}

Optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    struct stat fileStat;
    if (stat(fsFile.data(), &fileStat) == -1)
        return WTF::nullopt;

    return fileStat.st_dev;
}

#if OS(MORPHOS)
String realPath(const String& filePath)
{
    CString fsRep = fileSystemRepresentation(filePath);
    char resolvedName[PATH_MAX];
    const char* result = realpath(fsRep.data(), resolvedName);
    return result ? String::fromUTF8(result) : filePath;
}
#endif

} // namespace FileSystemImpl
} // namespace WTF
