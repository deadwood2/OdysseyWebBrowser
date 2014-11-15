/* WebKitAdBlock
 *
 * Copyright (C) 2009 Jonah Sherman <sherman.jonah@gmail.com>
 * Based on AdBlockPlus by Wladimir Palant <trev@adblockplus.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND ITS CONTRIBUTORS "AS IS" AND ANY
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
#include "CachedResource.h"
#include <wtf/text/CString.h>
#include "TextEncoding.h"
#include <yarr/RegularExpression.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/StringView.h>

#include <stdio.h>
#include <clib/debug_protos.h>
#include "gui.h"

namespace WebCore {

#define DOCUMENT_TYPE 9
#define CACHE_SIZE 1009
#define SHORTCUT_SIZE 8
#define FILTER_PATH "PROGDIR:conf/blocked.prefs"

class AdPattern {
public:
	AdPattern() : m_string(""), m_re(JSC::Yarr::RegularExpression("", TextCaseInsensitive)), m_types(0) { }
	AdPattern(String str, const JSC::Yarr::RegularExpression& re, int types)
		: m_string(str), m_re(re), m_types(types) { }
    bool matches(const String& target, int type) {
		return ((1<<type) & m_types) && m_re.match(target) >= 0;
    }
	String m_string;
    JSC::Yarr::RegularExpression m_re;
private:
    unsigned int m_types;
};

class CacheEntry {
public:
    String target;
    bool block;
};

class PatternMatcher {
public:
	AdPattern* addPattern(const String& pat);
	bool updatePattern(const String& pat, AdPattern* newpattern);
    bool matches(const String& target, int type);
	Vector<AdPattern *>* patterns() { return &m_patterns; }
private:
	Vector<AdPattern *> m_patterns;
//	  HashMap<String, AdPattern> m_shortcuts;
};

bool ad_block_enabled = false;
static CacheEntry *ab_cache;
static PatternMatcher ab_blackList;
static PatternMatcher ab_whiteList;

AdPattern* PatternMatcher::addPattern(const String& pat)
{
	AdPattern *ret = 0;
	size_t delim = pat.find("#");
    String pattern, optpart;
	if (delim == notFound) {
        delim = pat.length();
    }
    optpart = pat.substring(delim+1);
    pattern = pat.left(delim);

    Vector<String> opts;
    optpart.split(",", opts);
    int types = -1;
    for (unsigned i = 0; i < opts.size(); i++) {
        const String &opt = opts[i];
        if (opts[i] == "match-case") {
            continue;
        }
        bool invert = false;
        String typeOpt = opt;
        int typeMask = 0;
        if (typeOpt.startsWith("~")) {
            invert = true;
            typeOpt = typeOpt.substring(1);
        }
        if (typeOpt == "image") {
            typeMask = 1<<CachedResource::ImageResource;
        } else if (typeOpt == "stylesheet") {
            typeMask = 1<<CachedResource::CSSStyleSheet;
        } else if (typeOpt == "script") {
            typeMask = 1<<CachedResource::Script;
        } else if (typeOpt == "subdocument") {
            typeMask = 1<<DOCUMENT_TYPE;
        }
        if (invert) {
            types &= ~typeMask;
        } else {
            if (types == -1) {
                types = 0;
            }
            types |= typeMask;
        }
    }

    if (!types) {
		return 0;
    }
    if (pattern.startsWith("/") && pattern.endsWith("/")) {
        pattern = pattern.substring(1, pattern.length()-2);
		ret = new AdPattern(pat, JSC::Yarr::RegularExpression(pattern, TextCaseInsensitive), types);
		m_patterns.append(ret);
		return ret;
    }
    int pos = 0;
	JSC::Yarr::RegularExpression nonchar("\\W", TextCaseInsensitive);
	while ((pos = nonchar.match(pattern, pos)) >= 0) {
        pattern.insert("\\", pos);
        pos += 2;
    }

	replace(pattern, JSC::Yarr::RegularExpression("\\\\\\*", TextCaseInsensitive), ".*");
	replace(pattern, JSC::Yarr::RegularExpression("^\\\\\\|", TextCaseInsensitive), "^");
	replace(pattern, JSC::Yarr::RegularExpression("\\\\\\|$", TextCaseInsensitive), "$");

	/*
    String text = pat.left(delim).lower();
	replace(text, RegularExpression("^\\|", TextCaseInsensitive), "");
	replace(text, RegularExpression("\\|$", TextCaseInsensitive), "");
    for (pos = text.length() - SHORTCUT_SIZE; pos >= 0; pos--) {
        String sub = text.substring(pos, SHORTCUT_SIZE);
        if (sub.find("*") < 0 && !m_shortcuts.contains(sub)) {
			m_shortcuts.set(sub, AdPattern(pat, RegularExpression(pattern, TextCaseInsensitive), types));
            return;
        }
    }
	*/
	ret = new AdPattern(pat, JSC::Yarr::RegularExpression(pattern, TextCaseInsensitive), types);
	m_patterns.append(ret);
	return ret;
}

bool PatternMatcher::updatePattern(const String& pat, AdPattern* newpattern)
{
	size_t delim = pat.find("#");
    String pattern, optpart;
	if (delim == notFound) {
        delim = pat.length();
    }
    optpart = pat.substring(delim+1);
    pattern = pat.left(delim);

    Vector<String> opts;
    optpart.split(",", opts);
    int types = -1;
    for (unsigned i = 0; i < opts.size(); i++) {
        const String &opt = opts[i];
        if (opts[i] == "match-case") {
            continue;
        }
        bool invert = false;
        String typeOpt = opt;
        int typeMask = 0;
        if (typeOpt.startsWith("~")) {
            invert = true;
            typeOpt = typeOpt.substring(1);
        }
        if (typeOpt == "image") {
            typeMask = 1<<CachedResource::ImageResource;
        } else if (typeOpt == "stylesheet") {
            typeMask = 1<<CachedResource::CSSStyleSheet;
        } else if (typeOpt == "script") {
            typeMask = 1<<CachedResource::Script;
        } else if (typeOpt == "subdocument") {
            typeMask = 1<<DOCUMENT_TYPE;
        }
        if (invert) {
            types &= ~typeMask;
        } else {
            if (types == -1) {
                types = 0;
            }
            types |= typeMask;
        }
    }

    if (!types) {
		return false;
    }
    if (pattern.startsWith("/") && pattern.endsWith("/")) {
        pattern = pattern.substring(1, pattern.length()-2);
		*newpattern = AdPattern(pat, JSC::Yarr::RegularExpression(pattern, TextCaseInsensitive), types);
		return true;
    }
    int pos = 0;
	JSC::Yarr::RegularExpression nonchar("\\W", TextCaseInsensitive);
	while ((pos = nonchar.match(pattern, pos)) >= 0) {
        pattern.insert("\\", pos);
        pos += 2;
    }

	replace(pattern, JSC::Yarr::RegularExpression("\\\\\\*", TextCaseInsensitive), ".*");
	replace(pattern, JSC::Yarr::RegularExpression("^\\\\\\|", TextCaseInsensitive), "^");
	replace(pattern, JSC::Yarr::RegularExpression("\\\\\\|$", TextCaseInsensitive), "$");

	/*
    String text = pat.left(delim).lower();
	replace(text, RegularExpression("^\\|", TextCaseInsensitive), "");
	replace(text, RegularExpression("\\|$", TextCaseInsensitive), "");
    for (pos = text.length() - SHORTCUT_SIZE; pos >= 0; pos--) {
        String sub = text.substring(pos, SHORTCUT_SIZE);
        if (sub.find("*") < 0 && !m_shortcuts.contains(sub)) {
			m_shortcuts.set(sub, AdPattern(pat, RegularExpression(pattern, TextCaseInsensitive), types));
            return;
        }
    }
	*/
	*newpattern = AdPattern(pat, JSC::Yarr::RegularExpression(pattern, TextCaseInsensitive), types);
	return true;
}

bool PatternMatcher::matches(const String& target, int type)
{
    int i;
	/*
    for (i = target.length() - SHORTCUT_SIZE; i >= 0; i--) {
        String sub = target.substring(i, SHORTCUT_SIZE);
		AdPattern pattern = m_shortcuts.get(sub);
        if (pattern.matches(target, type)) {
            return true;

        }
    }
	*/
    int size = m_patterns.size();
    for (i = 0; i < size; i++) {
		if (m_patterns[i]->matches(target, type)) {
            return true;
        }
    }
    return false;
}

// XXX: Figure out how to use existing String hash buried in a nest of templates
static
unsigned ab_hash(const String& str)
{
    unsigned result = 0;
    unsigned size = str.length();
    auto upconvertedCharacters = StringView(str).upconvertedCharacters();
    const UChar *p = upconvertedCharacters;
    for (unsigned i = 0; i < size; i++) {
        result = 31 * result + (unsigned)p[i];
    }
    return result;
}


static CacheEntry* initialize()
{
    FILE *file = fopen(FILTER_PATH, "r");
    if (file) {
        char buf[512];
        fgets(buf, 512, file);
		while (fgets(buf, 512, file))
		{
            String line(buf);
            line.replace("\n","");
			if (line.startsWith("@@"))
			{
				AdPattern *pattern = ab_whiteList.addPattern(line.substring(2));
				DoMethod(app, MM_BlockManagerGroup_DidInsert, line.substring(2).utf8().data(), 1, (void *) pattern);
			}
			else if (!line.startsWith("!") && !line.startsWith("#") && !line.isEmpty())
			{
				AdPattern *pattern = ab_blackList.addPattern(line);
				DoMethod(app, MM_BlockManagerGroup_DidInsert, line.utf8().data(), 0, (void *) pattern);
            }
        }
        fclose(file);
    }
    return new CacheEntry[CACHE_SIZE];
}

void deinitialize()
{
	delete [] ab_cache;
	ab_cache = 0;

	for(size_t i = 0; i < (*ab_whiteList.patterns()).size(); i++)
	{
		delete (*ab_whiteList.patterns())[i];
	}

	for(size_t i = 0; i < (*ab_blackList.patterns()).size(); i++)
	{
		delete (*ab_blackList.patterns())[i];
	}
}

void flushCache()
{
	if(ab_cache)
	{
		delete [] ab_cache;
	}
    ab_cache = new CacheEntry[CACHE_SIZE];
}

void loadCache()
{
	if(!ab_cache)
	{
		ab_cache = initialize();
	} 
}

bool writeCache()
{
	FILE *file = fopen(FILTER_PATH, "w");
	if (file)
	{
	    if (!ab_cache) {
	        ab_cache = initialize();
	    }

		fprintf(file, "[Adblock]\n");
		fprintf(file, "!---- Generated by OWB ----!\n");

		fprintf(file, "!---- White List ----!\n");
		for(size_t i = 0; i < (*ab_whiteList.patterns()).size(); i++)
		{
			fprintf(file, "@@%s\n", (*ab_whiteList.patterns())[i]->m_string.latin1().data());
		}

		fprintf(file, "!---- Black List ----!\n");
		for(size_t i = 0; i < (*ab_blackList.patterns()).size(); i++)
		{
			fprintf(file, "%s\n", (*ab_blackList.patterns())[i]->m_string.latin1().data());
		}

        fclose(file);

		return true;
    }

	return false;
}

void *addCacheEntry(String rule, int type)
{
	if(type == 0)
	{
		return ab_blackList.addPattern(rule);
	}
	else if(type == 1)
	{
		return ab_whiteList.addPattern(rule);
	}
	return NULL;
}

void updateCacheEntry(String rule, int type, void *ptr)
{
	if(type == 0)
	{
		for(size_t i = 0; i < (*ab_blackList.patterns()).size(); i++)
		{
			if((void *) (*ab_blackList.patterns())[i] == ptr)
			{
				ab_blackList.updatePattern(rule, (*ab_blackList.patterns())[i]);
				break;
			}
		}
	}
	else if(type == 1)
	{
		for(size_t i = 0; i < (*ab_whiteList.patterns()).size(); i++)
		{
			if((void *) (*ab_whiteList.patterns())[i] == ptr)
			{
				ab_whiteList.updatePattern(rule, (*ab_whiteList.patterns())[i]);
				break;
			}
		}
	}
}


void removeCacheEntry(void *ptr, int type)
{
	if(type == 0)
	{
		for(size_t i = 0; i < (*ab_blackList.patterns()).size(); i++)
		{
			if((void *) (*ab_blackList.patterns())[i] == ptr)
			{
				(*ab_blackList.patterns()).remove(i);
				delete (AdPattern *)ptr;
				break;
			}
		}
	}
	else if(type == 1)
	{
		for(size_t i = 0; i < (*ab_whiteList.patterns()).size(); i++)
		{
			if((void *) (*ab_whiteList.patterns())[i] == ptr)
			{
				(*ab_whiteList.patterns()).remove(i);
				delete (AdPattern *)ptr;
				break;
			}
		}
	}
}

void blockResource(const URL& url, int type, int mode)
{
	String target = url.string();
	String typeOpt = "";
	String pat;

	if(type == 1<<CachedResource::ImageResource)
	{
		typeOpt = "image";
	}
	else if(type == 1<<CachedResource::CSSStyleSheet)
	{
		typeOpt = "stylesheet";
	}
	else if (type == 1<<CachedResource::Script)
	{
		typeOpt = "script";
	}
	else if(type == 1<<DOCUMENT_TYPE)
	{
		typeOpt = "subdocument";
	}

	switch(mode)
	{
		default:
		case 0:
			pat = url.string();
			break;
		case 1:
			pat = url.protocol();
			pat.append("://");
			pat.append(url.host());
			pat.append("/*");
			break;
		case 2:
		{
			String path = url.path();
			int lastSlashLocation = path.reverseFind('/');
			pat = url.protocol();
			pat.append("://");
			pat.append(url.host());

			if (lastSlashLocation > 0)
			{
			    pat.append(path.substring(0, lastSlashLocation));
			    pat.append("/*");
			}
			else
			{
			    pat.append("/*");
			}
			break;
		}
	}

	if(!typeOpt.isEmpty())
	{
	    pat.append("#");
	    pat.append(typeOpt);
	}

	AdPattern *pattern = ab_blackList.addPattern(pat);
	DoMethod(app, MM_BlockManagerGroup_DidInsert, pat.utf8().data(), 0, (void *) pattern);
	writeCache();
    flushCache();
}

bool shouldBlock(const URL& url, int type)
{
    if (url.protocolIs("data")) { return false; }
    if (!ad_block_enabled) { return false; }
    if (type < 0) { type = DOCUMENT_TYPE; }

    if (!ab_cache) {
        ab_cache = initialize();
    }
    String target = url.string();
    CacheEntry& ent = ab_cache[ab_hash(target) % CACHE_SIZE];
    if (ent.target != target) {
        ent.target = target;
        ent.block = (!ab_whiteList.matches(target, type))
                   && ab_blackList.matches(target, type);
    }
    return ent.block;
}

}
