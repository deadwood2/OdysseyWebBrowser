/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGPathUtilities.h"

#include "Path.h"
#include "PathTraversalState.h"
#include "SVGPathBlender.h"
#include "SVGPathBuilder.h"
#include "SVGPathByteStreamBuilder.h"
#include "SVGPathByteStreamSource.h"
#include "SVGPathConsumer.h"
#include "SVGPathElement.h"
#include "SVGPathParser.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListSource.h"
#include "SVGPathStringBuilder.h"
#include "SVGPathStringSource.h"
#include "SVGPathTraversalStateBuilder.h"

namespace WebCore {

bool buildPathFromString(const String& d, Path& result)
{
    if (d.isEmpty())
        return true;

    SVGPathBuilder builder(result);
    SVGPathStringSource source(d);
    return SVGPathParser::parse(source, builder);
}

bool buildSVGPathByteStreamFromSVGPathSegList(const SVGPathSegList& list, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    result.clear();
    if (list.isEmpty())
        return true;

    SVGPathSegListSource source(list);
    return SVGPathParser::parseToByteStream(source, result, parsingMode);
}

bool appendSVGPathByteStreamFromSVGPathSeg(RefPtr<SVGPathSeg>&& pathSeg, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    ASSERT(parsingMode == UnalteredParsing);

    SVGPathSegList appendedItemList(PathSegUnalteredRole);
    appendedItemList.append(WTFMove(pathSeg));

    SVGPathByteStream appendedByteStream;
    SVGPathSegListSource source(appendedItemList);
    bool ok = SVGPathParser::parseToByteStream(source, result, parsingMode, false);

    if (ok)
        result.append(appendedByteStream);

    return ok;
}

bool buildPathFromByteStream(const SVGPathByteStream& stream, Path& result)
{
    if (stream.isEmpty())
        return true;

    SVGPathBuilder builder(result);
    SVGPathByteStreamSource source(stream);
    return SVGPathParser::parse(source, builder);
}

bool buildSVGPathSegListFromByteStream(const SVGPathByteStream& stream, SVGPathElement& element, SVGPathSegList& result, PathParsingMode parsingMode)
{
    if (stream.isEmpty())
        return true;

    SVGPathSegListBuilder builder(element, result, parsingMode == NormalizedParsing ? PathSegNormalizedRole : PathSegUnalteredRole);
    SVGPathByteStreamSource source(stream);
    return SVGPathParser::parse(source, builder, parsingMode);
}

bool buildStringFromByteStream(const SVGPathByteStream& stream, String& result, PathParsingMode parsingMode)
{
    if (stream.isEmpty())
        return true;

    SVGPathByteStreamSource source(stream);
    return SVGPathParser::parseToString(source, result, parsingMode);
}

bool buildStringFromSVGPathSegList(const SVGPathSegList& list, String& result, PathParsingMode parsingMode)
{
    result = String();
    if (list.isEmpty())
        return true;

    SVGPathSegListSource source(list);
    return SVGPathParser::parseToString(source, result, parsingMode);
}

bool buildSVGPathByteStreamFromString(const String& d, SVGPathByteStream& result, PathParsingMode parsingMode)
{
    result.clear();
    if (d.isEmpty())
        return true;

    SVGPathStringSource source(d);
    return SVGPathParser::parseToByteStream(source, result, parsingMode);
}

bool canBlendSVGPathByteStreams(const SVGPathByteStream& fromStream, const SVGPathByteStream& toStream)
{
    SVGPathByteStreamSource fromSource(fromStream);
    SVGPathByteStreamSource toSource(toStream);
    return SVGPathBlender::canBlendPaths(fromSource, toSource);
}

bool buildAnimatedSVGPathByteStream(const SVGPathByteStream& fromStream, const SVGPathByteStream& toStream, SVGPathByteStream& result, float progress)
{
    ASSERT(&toStream != &result);
    result.clear();
    if (toStream.isEmpty())
        return true;

    SVGPathByteStreamBuilder builder(result);

    SVGPathByteStreamSource fromSource(fromStream);
    SVGPathByteStreamSource toSource(toStream);
    return SVGPathBlender::blendAnimatedPath(fromSource, toSource, builder, progress);
}

bool addToSVGPathByteStream(SVGPathByteStream& streamToAppendTo, const SVGPathByteStream& byStream, unsigned repeatCount)
{
    // Why return when streamToAppendTo is empty? Don't we still need to append?
    if (streamToAppendTo.isEmpty() || byStream.isEmpty())
        return true;

    // Is it OK to make the SVGPathByteStreamBuilder from a stream, and then clear that stream?
    SVGPathByteStreamBuilder builder(streamToAppendTo);

    SVGPathByteStream fromStreamCopy = streamToAppendTo;
    streamToAppendTo.clear();

    SVGPathByteStreamSource fromSource(fromStreamCopy);
    SVGPathByteStreamSource bySource(byStream);
    return SVGPathBlender::addAnimatedPath(fromSource, bySource, builder, repeatCount);
}

bool getSVGPathSegAtLengthFromSVGPathByteStream(const SVGPathByteStream& stream, float length, unsigned& pathSeg)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::SegmentAtLength);
    SVGPathTraversalStateBuilder builder(traversalState, length);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    pathSeg = builder.pathSegmentIndex();
    return ok;
}

bool getTotalLengthOfSVGPathByteStream(const SVGPathByteStream& stream, float& totalLength)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::TotalLength);

    SVGPathTraversalStateBuilder builder(traversalState);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    totalLength = builder.totalLength();
    return ok;
}

bool getPointAtLengthOfSVGPathByteStream(const SVGPathByteStream& stream, float length, SVGPoint& point)
{
    if (stream.isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::VectorAtLength);

    SVGPathTraversalStateBuilder builder(traversalState, length);

    SVGPathByteStreamSource source(stream);
    bool ok = SVGPathParser::parse(source, builder);
    point = builder.currentPoint();
    return ok;
}

static void pathIteratorForBuildingString(SVGPathConsumer& consumer, const PathElement& pathElement)
{
    switch (pathElement.type) {
    case PathElementMoveToPoint:
        consumer.moveTo(pathElement.points[0], false, AbsoluteCoordinates);
        break;
    case PathElementAddLineToPoint:
        consumer.lineTo(pathElement.points[0], AbsoluteCoordinates);
        break;
    case PathElementAddQuadCurveToPoint:
        consumer.curveToQuadratic(pathElement.points[0], pathElement.points[1], AbsoluteCoordinates);
        break;
    case PathElementAddCurveToPoint:
        consumer.curveToCubic(pathElement.points[0], pathElement.points[1], pathElement.points[2], AbsoluteCoordinates);
        break;
    case PathElementCloseSubpath:
        consumer.closePath();
        break;

    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

bool buildStringFromPath(const Path& path, String& string)
{
    // Ideally we would have a SVGPathPlatformPathSource, but it's not possible to manually iterate
    // a path, only apply a function to all path elements at once.

    SVGPathStringBuilder builder;
    path.apply([&builder](const PathElement& pathElement) {
        pathIteratorForBuildingString(builder, pathElement);
    });
    string = builder.result();

    return true;
}

}
