/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

WI.CPUUsageView = class CPUUsageView extends WI.View
{
    constructor()
    {
        super();

        this.element.classList.add("cpu-usage-view");

        this._detailsElement = this.element.appendChild(document.createElement("div"));
        this._detailsElement.classList.add("details");

        this._detailsAverageElement = this._detailsElement.appendChild(document.createElement("span"));
        this._detailsElement.appendChild(document.createElement("br"));
        this._detailsMaxElement = this._detailsElement.appendChild(document.createElement("span"));
        this._detailsElement.appendChild(document.createElement("br"));
        this._detailsMinElement = this._detailsElement.appendChild(document.createElement("span"));
        this._updateDetails(NaN, NaN);

        this._graphElement = this.element.appendChild(document.createElement("div"));
        this._graphElement.classList.add("graph");

        this._chart = new WI.LineChart;
        this.addSubview(this._chart);
        this._graphElement.appendChild(this._chart.element);
    }

    // Public

    clear()
    {
        this._cachedAverageSize = undefined;
        this._cachedMinSize = undefined;
        this._cachedMaxSize = undefined;
        this._updateDetails(NaN, NaN);

        this._chart.clear();
    }

    updateChart(dataPoints, size, visibleEndTime, min, max, average, xScale, yScale)
    {
        console.assert(size instanceof WI.Size);
        console.assert(min >= 0);
        console.assert(max >= 0);
        console.assert(min <= max);
        console.assert(min <= average && average <= max);

        this._updateDetails(min, max, average);

        this._chart.clear();
        this._chart.size = size;
        this._chart.needsLayout();

        if (!dataPoints.length)
            return;

        // Ensure an empty graph is empty.
        if (!max)
            return;

        // Extend the first data point to the start so it doesn't look like we originate at zero size.
        let firstX = 0;
        let firstY = yScale(dataPoints[0].size);
        this._chart.addPoint(firstX, firstY);

        // Points for data points.
        for (let dataPoint of dataPoints) {
            let x = xScale(dataPoint.time);
            let y = yScale(dataPoint.size);
            this._chart.addPoint(x, y);
        }

        // Extend the last data point to the end time.
        let lastDataPoint = dataPoints.lastValue;
        let lastX = Math.floor(xScale(visibleEndTime));
        let lastY = yScale(lastDataPoint.size);
        this._chart.addPoint(lastX, lastY);
    }

    // Private

    _updateDetails(minSize, maxSize, averageSize)
    {
        if (this._cachedMinSize === minSize && this._cachedMaxSize === maxSize && this._cachedAverageSize === averageSize)
            return;

        this._cachedAverageSize = averageSize;
        this._cachedMinSize = minSize;
        this._cachedMaxSize = maxSize;

        this._detailsAverageElement.textContent = WI.UIString("Average: %s").format(Number.isFinite(maxSize) ? Number.percentageString(averageSize / 100) : emDash);
        this._detailsMaxElement.textContent = WI.UIString("Highest: %s").format(Number.isFinite(maxSize) ? Number.percentageString(maxSize / 100) : emDash);
        this._detailsMinElement.textContent = WI.UIString("Lowest: %s").format(Number.isFinite(minSize) ? Number.percentageString(minSize / 100) : emDash);
    }
};
