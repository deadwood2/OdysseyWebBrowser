/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
#include "FileSystem.h"

#include "FileMetadata.h"
#include <errno.h>
#include <sys/stat.h>
#if PLATFORM(MUI)
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/wb.h>
#include <clib/debug_protos.h>
#include <sys/time.h>
#else
#include <libgen.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>
#include <string>
using namespace std;

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static String removeFilePrefix(const String& fileName)
{
    String correctedFileName(fileName);
	if (correctedFileName.startsWith("file://"))
        correctedFileName = correctedFileName.substring(7);

    return correctedFileName;
}

char* filenameFromString(const String& string)
{
#warning "Really?"
    return strdup(string.utf8().data());
}

String filenameToString(const char* filename)
{
#warning "Really?"
    if (!filename)
        return String();
    return String::fromUTF8(filename);
}

bool fileExists(const String& path)
{
    if (path.isNull())
        return false;

    String correctedPath = removeFilePrefix(path);

    CString fsRep = fileSystemRepresentation(correctedPath);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;

    // stat(...) returns 0 on successful stat'ing of the file, and non-zero in any case where the file doesn't exist or cannot be accessed
    return !stat(fsRep.data(), &fileInfo);
}

bool deleteFile(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    // unlink(...) returns 0 on successful deletion of the path and non-zero in any other case (including invalid permissions or non-existent file)
    return !unlink(fsRep.data());
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    CString fsRep = fileSystemRepresentation(path);

    if (fsRep.isNull())
        return invalidPlatformFileHandle;

    int platformFlag = 0;
    if (mode == OpenForRead)
        platformFlag |= O_RDONLY;
    else if (mode == OpenForWrite)
#warning "replaced O_TRUNC, else APPEND would never work. Ok for now given this class use, but check sideeffects when updating."
	platformFlag |= (O_WRONLY | O_CREAT | /*O_TRUNC*/ O_APPEND);
    return open(fsRep.data(), platformFlag, 0666);
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
    case SeekFromBeginning:
        whence = SEEK_SET;
        break;
    case SeekFromCurrent:
        whence = SEEK_CUR;
        break;
    case SeekFromEnd:
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
bool lockFile(PlatformFileHandle handle, FileLockMode lockMode)
{
    COMPILE_ASSERT(LOCK_SH == LockShared, LockSharedEncodingIsAsExpected);
    COMPILE_ASSERT(LOCK_EX == LockExclusive, LockExclusiveEncodingIsAsExpected);
    COMPILE_ASSERT(LOCK_NB == LockNonBlocking, LockNonBlockingEncodingIsAsExpected);
    int result = flock(handle, lockMode);
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

bool getFileCreationTime(const String& path, time_t& result)
{
#if OS(DARWIN) || OS(OPENBSD) || OS(NETBSD) || OS(FREEBSD)
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return false;

    result = fileInfo.st_birthtime;
    return true;
#else
    UNUSED_PARAM(path);
    UNUSED_PARAM(result);
    return false;
#endif
}

bool getFileModificationTime(const String& path, time_t& result)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;

    if (stat(fsRep.data(), &fileInfo))
        return false;

    result = fileInfo.st_mtime;
    return true;
}

bool getFileMetadata(const String& path, FileMetadata& metadata)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;
    if (stat(fsRep.data(), &fileInfo))
        return false;

    metadata.modificationTime = fileInfo.st_mtime;
    metadata.length = fileInfo.st_size;
    metadata.type = S_ISDIR(fileInfo.st_mode) ? FileMetadata::TypeDirectory : FileMetadata::TypeFile;
    return true;
}

String pathByAppendingComponent(const String& path, const String& component)
{
#warning "Just use AddPart..."
    if (path.endsWith('/'))
        return path + component;
#if PLATFORM(MUI)
    else if(path.endsWith(":"))
        return path + component;
#endif
    else
        return path + "/" + component;
}

bool makeAllDirectories(const String& path)
{
    CString fullPath = fileSystemRepresentation(path);
    if (!access(fullPath.data(), F_OK))
        return true;

    char* p = fullPath.mutableData() + 1;
    int length = fullPath.length();

    if(p[length - 1] == '/')
        p[length - 1] = '\0';
    for (; *p; ++p)
        if (*p == '/') {
            *p = '\0';
            if (access(fullPath.data(), F_OK))
                if (mkdir(fullPath.data(), S_IRWXU))
                    return false;
            *p = '/';
        }
    if (access(fullPath.data(), F_OK))
        if (mkdir(fullPath.data(), S_IRWXU))
            return false;

    return true;
}

#if PLATFORM(MUI)
static char *basename(char *path)
{
#warning "Just use FilePart/PathPart"
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  char *s1;
  char *s2;

  s1=strrchr(path, '/');
  s2=strrchr(path, ':');

  if(s1 && s2) {
    path = (s1 > s2? s1 : s2)+1;
  }
  else if(s1)
    path = s1 + 1;
  else if(s2)
    path = s2 + 1;

  return path;
}
#endif

String pathGetFileName(const String& path)
{
#if PLATFORM(MUI)
  return String(basename((char *) path.latin1().data()));
#else
  return path.substring(path.reverseFind('/') + 1);
#endif
}

#if PLATFORM(MUI)
// XXX: totally wrong of course
char *dirname (char *path)
{
#warning "Just use FilePart/PathPart"

  char *newpath;
  const char *slash;
  int length;                   /* Length of result, not including NUL.  */

  slash = strrchr (path, '/');
  if (slash == 0)
    {
      /* File is in the current directory.  */
      path = ".";
      length = 1;
    }
  else
    {
      /* Remove any trailing slashes from the result.  */
      while (slash > path && *slash == '/')
        --slash;

      length = slash - path + 1;
    }
  newpath = (char *) malloc (length + 1);
  if (newpath == 0)
    return 0;
  strncpy (newpath, path, length);
  newpath[length] = 0;
  return newpath;
}
#endif

String directoryName(const String& path)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return String();

	char* cname	= dirname(fsRep.mutableData());
	String ret = String(cname);
	free(cname);

	return ret;
}

#if !PLATFORM(EFL) && !PLATFORM(MUI)
Vector<String> listDirectory(const String& path, const String& filter)
{
    Vector<String> entries;
    CString cpath = path.utf8();
    CString cfilter = filter.utf8();
    DIR* dir = opendir(cpath.data());
    if (dir) {
      struct dirent* dp;
      while ((dp = readdir(dir))) {
	const char* name = dp->d_name;
	if (!strcmp(name, ".") || !strcmp(name, ".."))
	  continue;
	if (fnmatch(cfilter.data(), name, 0))
	  continue;
	char filePath[1024];
	if (static_cast<int>(sizeof(filePath) - 1) < snprintf(filePath, sizeof(filePath), "%s/%s", cpath.data(), name))
	  continue; // buffer overflow
	entries.append(filePath);
      }
      closedir(dir);
    }
    return entries;
  }
#else
Vector<String> listDirectory(const String& path, const String&)
{
    Vector<String> entries;

    char* filename = filenameFromString(path);
    DIR* dir = opendir(filename);
    if (!dir)
        return entries;
    /*int err;

    regex_t preg;
	char *str_regex = strdup(filter.utf8().data());
    err = regcomp (&preg, str_regex, REG_NOSUB | REG_EXTENDED | REG_NEWLINE);
    if (err != 0) {
        return entries;
    }
	*/
    struct dirent* file;
    /* int match;*/
    while ((file = readdir(dir))) {
		/*
        match = regexec (&preg, file->d_name, 0, NULL, 0);
        if (match != 0)
            continue;
		*/
		String s = filename;
		s = pathByAppendingComponent(s, file->d_name);
		entries.append(s);
    }
	/*
	regfree (&preg);
    free(str_regex);
	*/
    closedir(dir);
    free(filename);

    return entries;
}
#endif

void revealFolderInOS(const String& path)
{
    OpenWorkbenchObjectA((char *)path.utf8().data(), NULL); 
}

CString fileSystemRepresentation(const String &file)
{
    return file.utf8();
}

bool unloadModule(PlatformModule module)
{
	CloseLibrary((struct Library *) module);
	return true;
}

String homeDirectoryPath()
{
	return "PROGDIR:";
}

String openTemporaryFile(const String& prefix, PlatformFileHandle &handle)
{
	char tempPath[1024];
	struct timeval tv;

	gettimeofday(&tv, NULL);

	snprintf(tempPath, sizeof(tempPath), "T:%s_%lu", prefix.utf8().data(), (unsigned long)(tv.tv_sec + 1000000*tv.tv_usec));

	int fileDescriptor = open(tempPath, O_CREAT | O_RDWR);

	if (!isHandleValid(fileDescriptor))
	 {
		kprintf("Can't create temporary file <%s>\n", tempPath);
                return String();
         }
    String tempFilePath = tempPath;

    handle = fileDescriptor;
    return tempFilePath;
}

} // namespace WebCore
