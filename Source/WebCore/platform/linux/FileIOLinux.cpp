/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "FileIOLinux.h"
#include <wtf/text/CString.h>

#include "owb-config.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#if PLATFORM(MUI)
#include <dos/dos.h>
#include <proto/dos.h>
#if !OS(AROS)
#include <proto/asyncio.h>
#endif
#include <clib/debug_protos.h>
#define D(x)
#endif

namespace WebCore {

OWBFile::OWBFile(const String path)
	: m_fd(0)
    , m_filePath(path)
{
	D(kprintf("[File] File(%s) (0x%p)\n", m_filePath.latin1().data(), m_fd));
}

OWBFile::~OWBFile()
{
	D(kprintf("[File] ~File(%s) (0x%p)\n", m_filePath.latin1().data(), m_fd));
	close();
	D(kprintf("[File] ~File() OK\n"));
}

#if !OS(AROS)
int OWBFile::open(char openType)
{
	struct AsyncFile *fd = 0;
	char name[PATH_MAX];
	
	stccpy(name, m_filePath.latin1().data(), sizeof(name));

	D(kprintf("[File] open(%s, '%c')\n", name, openType));

	// Check that m_fd is 0, if that's not the case it means that an open file
    // has not been closed, so close it before continuing.
	if (m_fd)
        close();

    if (openType == 'w')
	{
		BPTR l;
		if((l=Lock(name, ACCESS_READ)))
		{
			UnLock(l);
			DeleteFile(name);
		}
		fd = OpenAsync(name, MODE_APPEND, 65536);
		if(fd)
		{
			SeekAsync(fd, 0, MODE_START);
		}
	}
    else if (openType == 'r')
	{
		fd = OpenAsync(name, MODE_READ, 65536);
	}
	else if (openType == 'a')
	{
		fd = OpenAsync(name, MODE_APPEND, 65536);
		if(fd)
		{
			SeekAsync(fd, 0, MODE_END);
		}
	}

	if(fd)
	{
		m_fd = (void *) fd;
		D(kprintf("[File] Opened <%s> successfully (0x%p)\n", m_filePath.latin1().data(), m_fd));
	}
	else
	{
		char str[128];
		Fault(IoErr(), NULL, str, sizeof(str));
		printf("[File] Can't open file '%s' in '%c' mode. Error: %s\n", name, openType, str);
	}

	return (m_fd != 0) ? (int) m_fd : -1;
}

void OWBFile::close()
{
	if (m_fd)
	{
		if((int) m_fd < 0x1000)
		{
			kprintf("[File] m_fd 0x%p is highly suspicious, skipping\n", m_fd);
			kprintf("[File] Dumping task state\n");
			DumpTaskState(FindTask(NULL));
			m_fd = 0;
			return;
		}

		D(kprintf("[File] close(%s) (0x%p)\n", m_filePath.latin1().data(), m_fd));
		CloseAsync((struct AsyncFile *) m_fd);
		m_fd = 0;
    }
}

char* OWBFile::read(size_t size)
{
    // Check that we try to read on a valid file descriptor.
	if (m_fd)
	{
	    char* readData = new char[size + 1];
		ReadAsync((struct AsyncFile *) m_fd, readData, size);
	    readData[size] = '\0';
	    return readData;
	}
	else
	{
		return NULL;
	}
}

void OWBFile::write(String dataToWrite)
{
    // Check that we try to write on a valid file descriptor.
	if(m_fd)
	{
                char *data = strdup(dataToWrite.utf8().data()); // XXX: might not be needed, but weird trashes sometimes...
		int len = dataToWrite.length();
		if(data)
		{
		    WriteAsync((struct AsyncFile *) m_fd, (APTR) data, len);
		    //WriteAsync((struct AsyncFile *) m_fd,(APTR) dataToWrite.utf8().data(), dataToWrite.length());
		    free(data);
		}
	}
}

void OWBFile::write(const void *data, size_t length)
{
    // Check that we try to write on a valid file descriptor.
	if(m_fd)
	{
		WriteAsync((struct AsyncFile *) m_fd, (APTR) data, length);
	}
}

int OWBFile::getSize()
{
	int	fileSize, current;
    // If file descriptor is not valid then return a size of -1
	if (m_fd)
	{
	    //save the current offset
		current = (int) SeekAsync((struct AsyncFile *) m_fd, 0, MODE_END);
		fileSize = (int) SeekAsync((struct AsyncFile *) m_fd, current, MODE_START);

	    return fileSize;
	}
	else
	{
		return -1;
	}
}
#else
int OWBFile::open(char openType)
{
    char name[PATH_MAX];

    stccpy(name, m_filePath.latin1().data(), sizeof(name));

    if (openType == 'w')
        m_fd = ::fopen(name, "w");
    else if (openType == 'r')
        m_fd = ::fopen(name, "r");

    if (m_fd) return 0;

    return -1;
}

void OWBFile::close()
{
    if (m_fd)
        ::fclose(m_fd);
    m_fd = 0;
}

char* OWBFile::read(size_t size)
{
    char* readData = new char[size + 1];
    ::fread(readData, size, 1, m_fd);
    readData[size] = '\0';
    return readData;
}

void OWBFile::write(String dataToWrite)
{
    ::fwrite(dataToWrite.utf8().data(), dataToWrite.length(), 1, m_fd);
}

void OWBFile::write(const void* data, size_t length)
{
    ::fwrite(data, length, 1, m_fd);
}

int OWBFile::getSize()
{
    int fileSize, current;

    //save the current offset
    current = ftell(m_fd);

    //save the size
    fseek(m_fd, 0, SEEK_END);
    fileSize = ftell(m_fd);

    //go back to the previous offset
    fseek(m_fd, current, SEEK_SET);

    return fileSize;
}
#endif

}
