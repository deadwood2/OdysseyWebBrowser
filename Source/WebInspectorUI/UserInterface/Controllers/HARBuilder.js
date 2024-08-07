/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

// HTTP Archive (HAR) format - Version 1.2
// https://dvcs.w3.org/hg/webperf/raw-file/tip/specs/HAR/Overview.html#sec-har-object-types-creator
// http://www.softwareishard.com/blog/har-12-spec/

WI.HARBuilder = class HARBuilder
{
    static async buildArchive(resources)
    {
        let promises = [];
        for (let resource of resources) {
            console.assert(resource.finished);
            promises.push(new Promise((resolve, reject) => {
                // Always resolve.
                resource.requestContent().then(
                    (x) => resolve(x),
                    () => resolve(null)
                );
            }));
        }

        let contents = await Promise.all(promises);
        console.assert(contents.length === resources.length);

        return {
            log: {
                version: "1.2",
                creator: HARBuilder.creator(),
                pages: HARBuilder.pages(),
                entries: resources.map((resource, index) => HARBuilder.entry(resource, contents[index])),
            }
        };
    }

    static creator()
    {
        return {
            name: "WebKit Web Inspector",
            version: WI.Platform.version.build || "1.0",
        };
    }

    static pages()
    {
        return [{
            startedDateTime: HARBuilder.date(WI.frameResourceManager.mainFrame.mainResource.requestSentDate),
            id: "page_0",
            title: WI.frameResourceManager.mainFrame.url || "",
            pageTimings: HARBuilder.pageTimings(),
        }];
    }

    static pageTimings()
    {
        let result = {};

        let domContentReadyEventTimestamp = WI.frameResourceManager.mainFrame.domContentReadyEventTimestamp;
        if (!isNaN(domContentReadyEventTimestamp))
            result.onContentLoad = domContentReadyEventTimestamp * 1000;

        let loadEventTimestamp = WI.frameResourceManager.mainFrame.loadEventTimestamp;
        if (!isNaN(loadEventTimestamp))
            result.onLoad = loadEventTimestamp * 1000;

        return result;
    }

    static entry(resource, content)
    {
        let entry = {
            pageref: "page_0",
            startedDateTime: HARBuilder.date(resource.requestSentDate),
            time: 0,
            request: HARBuilder.request(resource),
            response: HARBuilder.response(resource, content),
            cache: HARBuilder.cache(resource),
            timings: HARBuilder.timings(resource),
        };

        if (resource.timingData.startTime && resource.timingData.responseEnd)
            entry.time = (resource.timingData.responseEnd - resource.timingData.startTime) * 1000;
        if (resource.remoteAddress)
            entry.serverIPAddress = HARBuilder.ipAddress(resource.remoteAddress);
        if (resource.connectionIdentifier)
            entry.connection = "" + resource.connectionIdentifier;

        // CFNetwork Custom Field `_fetchType`.
        if (resource.responseSource !== WI.Resource.ResponseSource.Unknown)
            entry._fetchType = HARBuilder.fetchType(resource.responseSource);

        return entry;
    }

    static request(resource)
    {
        let result = {
            method: resource.requestMethod || "",
            url: resource.url || "",
            httpVersion: WI.Resource.displayNameForProtocol(resource.protocol) || "",
            cookies: HARBuilder.cookies(resource.requestCookies, null),
            headers: HARBuilder.headers(resource.requestHeaders),
            queryString: resource.queryStringParameters || [],
            headersSize: !isNaN(resource.requestHeadersTransferSize) ? resource.requestHeadersTransferSize : -1,
            bodySize: !isNaN(resource.requestBodyTransferSize) ? resource.requestBodyTransferSize : -1,
        };

        if (resource.requestData)
            result.postData = HARBuilder.postData(resource);

        return result;
    }

    static response(resource, content)
    {
        let result = {
            status: resource.statusCode || 0,
            statusText: resource.statusText || "",
            httpVersion: WI.Resource.displayNameForProtocol(resource.protocol) || "",
            cookies: HARBuilder.cookies(resource.responseCookies, resource.requestSentDate),
            headers: HARBuilder.headers(resource.responseHeaders),
            content: HARBuilder.content(resource, content),
            redirectURL: resource.responseHeaders.valueForCaseInsensitiveKey("Location") || "",
            headersSize: !isNaN(resource.responseHeadersTransferSize) ? resource.responseHeadersTransferSize : -1,
            bodySize: !isNaN(resource.responseBodyTransferSize) ? resource.responseBodyTransferSize : -1,
        };

        // Chrome Custom Field `_transferSize`.
        if (!isNaN(resource.networkTotalTransferSize))
            result._transferSize = resource.networkTotalTransferSize;

        // Chrome Custom Field `_error`.
        if (resource.failureReasonText)
            result._error = resource.failureReasonText;

        return result;
    }

    static cookies(cookies, requestSentDate)
    {
        let result = [];

        for (let cookie of cookies) {
            let json = {
                name: cookie.name,
                value: cookie.value,
            };

            if (cookie.type === WI.Cookie.Type.Response) {
                if (cookie.path)
                    json.path = cookie.path;
                if (cookie.domain)
                    json.domain = cookie.domain;
                json.expires = HARBuilder.date(cookie.expirationDate(requestSentDate));
                json.httpOnly = cookie.httpOnly;
                json.secure = cookie.secure;
            }

            result.push(json);
        }

        return result;
    }

    static headers(headers)
    {
        let result = [];

        for (let key in headers)
            result.push({name: key, value: headers[key]});

        return result;
    }

    static content(resource, content)
    {
        let encodedSize = !isNaN(resource.networkEncodedSize) ? resource.networkEncodedSize : resource.estimatedNetworkEncodedSize;
        let decodedSize = !isNaN(resource.networkDecodedSize) ? resource.networkDecodedSize : resource.size;

        if (isNaN(decodedSize))
            decodedSize = 0;
        if (isNaN(encodedSize))
            encodedSize = 0;

        let result = {
            size: decodedSize,
            compression: decodedSize - encodedSize,
            mimeType: resource.mimeType || "x-unknown",
        };

        if (content) {
            if (content.rawContent)
                result.text = content.rawContent;
            if (content.rawBase64Encoded)
                result.encoding = "base64";
        }

        return result;
    }

    static postData(resource)
    {
        return {
            mimeType: resource.requestDataContentType || "",
            text: resource.requestData,
            params: resource.requestFormParameters || [],
        };
    }

    static cache(resource)
    {
        // FIXME: <https://webkit.org/b/178682> Web Inspector: Include <cache> details in HAR Export
        // http://www.softwareishard.com/blog/har-12-spec/#cache
        return {};
    }

    static timings(resource)
    {
        // Chrome has Custom Fields `_blocked_queueing` and `_blocked_proxy`.

        let result = {
            blocked: -1,
            dns: -1,
            connect: -1,
            ssl: -1,
            send: 0,
            wait: 0,
            receive: 0,
        };

        if (resource.timingData.startTime && resource.timingData.responseEnd) {
            let {startTime, domainLookupStart, domainLookupEnd, connectStart, connectEnd, secureConnectionStart, requestStart, responseStart, responseEnd} = resource.timingData;
            result.blocked = ((domainLookupStart || connectStart || requestStart) - startTime) * 1000;
            if (domainLookupStart)
                result.dns = ((domainLookupEnd || connectStart || requestStart) - domainLookupStart) * 1000;
            if (connectStart)
                result.connect = ((connectEnd || requestStart) - connectStart) * 1000;
            if (secureConnectionStart)
                result.ssl = ((connectEnd || requestStart) - secureConnectionStart) * 1000;
            result.send = (requestStart - (connectEnd || domainLookupEnd || startTime)) * 1000;
            result.wait = (responseStart - requestStart) * 1000;
            result.receive = (responseEnd - responseStart) * 1000;
        }

        return result;
    }

    // Helpers

    static ipAddress(remoteAddress)
    {
        // IP Address, without port.
        if (!remoteAddress)
            return "";

        // NOTE: Resource.remoteAddress always includes the port at the end.
        // So this always strips the last part.
        return remoteAddress.replace(/:\d+$/, "");
    }

    static date(date)
    {
        // ISO 8601
        if (!date)
            return "";

        return date.toISOString();
    }

    static fetchType(responseSource)
    {
        switch (responseSource) {
        case WI.Resource.ResponseSource.Network:
            return "Network Load";
        case WI.Resource.ResponseSource.MemoryCache:
            return "Memory Cache";
        case WI.Resource.ResponseSource.DiskCache:
            return "Disk Cache";
        case WI.Resource.ResponseSource.ServiceWorker:
            return "Service Worker";
        }

        console.assert(false);
        return undefined;
    }
}
