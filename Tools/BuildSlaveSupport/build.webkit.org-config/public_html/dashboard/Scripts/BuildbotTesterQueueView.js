/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

BuildbotTesterQueueView = function(debugQueues, releaseQueues)
{
    BuildbotQueueView.call(this, debugQueues, releaseQueues);

    this.update();
};

BaseObject.addConstructorFunctions(BuildbotTesterQueueView);

BuildbotTesterQueueView.prototype = {
    constructor: BuildbotTesterQueueView,
    __proto__: BuildbotQueueView.prototype,

    update: function()
    {
        BuildbotQueueView.prototype.update.call(this);

        this.element.removeChildren();

        function appendBuilderQueueStatus(queue)
        {
            if (queue.buildbot.needsAuthentication && !queue.buildbot.isAuthenticated) {
                this._appendUnauthorizedLineView(queue);
                return;
            }

            this._appendPendingRevisionCount(queue);

            var appendedStatus = false;

            var limit = 2;
            for (var i = 0; i < queue.iterations.length && limit > 0; ++i) {
                var iteration = queue.iterations[i];
                if (!iteration.loaded || !iteration.finished)
                    continue;

                --limit;

                var willHaveAnotherStatusLine = i + 1 < queue.iterations.length && limit > 0 && !iteration.successful; // This is not 100% correct, as the remaining iterations may not be finished or loaded yet, but close enough.
                var messageElement = this.revisionContentForIteration(iteration, (iteration.productive && willHaveAnotherStatusLine) ? iteration.previousProductiveIteration : null);

                if (iteration.successful) {
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var status = new StatusLineView(messageElement, StatusLineView.Status.Good, "all tests passed", undefined, url);
                    limit = 0;
                } else if (!iteration.productive) {
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var status = new StatusLineView(messageElement, StatusLineView.Status.Danger, iteration.text, undefined, url);
                } else if (iteration.failedTestSteps.length === 0) {
                    // Something wrong happened, but it was not a test failure.
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var status = new StatusLineView(messageElement, StatusLineView.Status.Danger, iteration.text, undefined, url);
                } else if (queue.crashesOnly) {
                    // A crashes-only queue is a queue where we are only interested in crashes, e.g. a GuardMalloc or an ASan one.
                    // Currently, only layout tests are supported in such.
                    var layoutTestResults = iteration.layoutTestResults;
                    if (layoutTestResults.tooManyFailures) {
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Danger, "failure limit exceeded", undefined, iteration.queue.buildbot.layoutTestResultsURLForIteration(iteration));
                        new PopoverTracker(status.statusBubbleElement, this._presentPopoverForLayoutTestRegressions.bind(this), iteration);
                    } else if (layoutTestResults.errorOccurred) {
                        var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Danger, iteration.text, undefined, url);
                    } else if (!layoutTestResults.crashCount) {
                        var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Good, "no crashes found", undefined, url);
                        limit = 0;
                    } else {
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Bad, layoutTestResults.crashCount === 1 ? "crash found" : "crashes found", layoutTestResults.crashCount, iteration.queue.buildbot.layoutTestResultsURLForIteration(iteration));
                        new PopoverTracker(status.statusBubbleElement, this._presentPopoverForLayoutTestRegressions.bind(this), iteration);
                    }
                } else if (iteration.failedTestSteps.length === 1) {
                    var failedStep = iteration.failedTestSteps[0];
                    if (failedStep.name === "layout-test") {
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Bad, this._testStepFailureDescription(failedStep), failedStep.tooManyFailures ? failedStep.failureCount + "\uff0b" : failedStep.failureCount, iteration.queue.buildbot.layoutTestResultsURLForIteration(iteration));
                        new PopoverTracker(status.statusBubbleElement, this._presentPopoverForLayoutTestRegressions.bind(this), iteration);
                    } else {
                        var status = new StatusLineView(messageElement, StatusLineView.Status.Bad, this._testStepFailureDescription(failedStep), failedStep.failureCount ? failedStep.failureCount : undefined, failedStep.URL);
                        new PopoverTracker(status.statusBubbleElement, this._presentPopoverForGenericTestFailures.bind(this), iteration);
                    }
                } else {
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var failureDescriptions = iteration.failedTestSteps.map(function(failedStep) { return this._testStepFailureDescriptionWithCount(failedStep) }, this);
                    var status = new StatusLineView(messageElement, StatusLineView.Status.Bad, failureDescriptions.join(", "), undefined, url);
                    new PopoverTracker(status.statusBubbleElement, this._presentPopoverForGenericTestFailures.bind(this), iteration);
                }

                this.element.appendChild(status.element);
                appendedStatus = true;
            }

            if (!appendedStatus) {
                var status = new StatusLineView("unknown", StatusLineView.Status.Neutral, "last passing build");
                this.element.appendChild(status.element);
            }
        }

        this.appendBuildStyle.call(this, this.releaseQueues, "Release", appendBuilderQueueStatus);
        this.appendBuildStyle.call(this, this.debugQueues, "Debug", appendBuilderQueueStatus);
    },

    _testStepFailureDescription: function(failedStep)
    {
        if (!failedStep.failureCount)
            return BuildbotIteration.TestSteps[failedStep.name] + " failed";
        if (failedStep.failureCount === 1)
            return BuildbotIteration.TestSteps[failedStep.name] + " failure";
        return BuildbotIteration.TestSteps[failedStep.name] + " failures";
    },

    _testStepFailureDescriptionWithCount: function(failedStep)
    {
        if (!failedStep.failureCount)
            return this._testStepFailureDescription(failedStep);
        if (failedStep.tooManyFailures) {
            // E.g. "50+ layout test failures", preventing line breaks around the "+".
            return failedStep.failureCount + "\ufeff\uff0b\u00a0" + this._testStepFailureDescription(failedStep);
        }
        // E.g. "1 layout test failure", preventing line break after the number.
        return failedStep.failureCount + "\u00a0" + this._testStepFailureDescription(failedStep);
    },

    _popoverContentForLayoutTestRegressions: function(iteration)
    {
        var hasTestHistory = typeof testHistory !== "undefined";

        var content = document.createElement("div");
        content.className = "test-results-popover";

        this._addIterationHeadingToPopover(iteration, content, "layout test failures");
        this._addDividerToPopover(content);

        if (!iteration.layoutTestResults.regressions) {
            var message = document.createElement("div");
            message.className = "loading-failure";
            message.textContent = "Test results couldn\u2019t be loaded";
            content.appendChild(message);
            return content;
        }

        function addFailureInfoLink(rowElement, className, text, url)
        {
            var linkElement = document.createElement("a");
            linkElement.className = className;
            linkElement.textContent = text;
            linkElement.href = url;
            linkElement.target = "_blank";
            rowElement.appendChild(linkElement);
        }

        function addFailureInfoText(rowElement, className, text)
        {
            var spanElement = document.createElement("span");
            spanElement.className = className;
            spanElement.textContent = text;
            rowElement.appendChild(spanElement);
        }

        var sortedRegressions = iteration.layoutTestResults.regressions.slice().sort(function(a, b) { return (a.path === b.path) ? 0 : (a.path > b.path) ? 1 : -1; });

        for (var i = 0, end = sortedRegressions.length; i != end; ++i) {
            var test = sortedRegressions[i];

            var rowElement = document.createElement("div");

            var testPathElement = document.createElement("span");
            testPathElement.className = "test-path";
            testPathElement.textContent = test.path;
            rowElement.appendChild(testPathElement);

            if (test.crash)
                addFailureInfoLink(rowElement, "failure-kind-indicator", "crash", iteration.queue.buildbot.layoutTestCrashLogURLForIteration(iteration, test.path));

            if (test.timeout)
                addFailureInfoText(rowElement, "failure-kind-indicator", "timeout");

            if (test.has_diff) {
                addFailureInfoLink(rowElement, "additional-link", "diff", iteration.queue.buildbot.layoutTestDiffURLForIteration(iteration, test.path));

                if (iteration.hasPrettyPatch)
                    addFailureInfoLink(rowElement, "additional-link", "pretty\xa0diff", iteration.queue.buildbot.layoutTestPrettyDiffURLForIteration(iteration, test.path));
            }

            if (test.has_image_diff) {
                addFailureInfoLink(rowElement, "additional-link", "images", iteration.queue.buildbot.layoutTestImagesURLForIteration(iteration, test.path));
                addFailureInfoLink(rowElement, "additional-link", "image\xa0diff", iteration.queue.buildbot.layoutTestImageDiffURLForIteration(iteration, test.path));
            }

            if (test.has_stderr)
                addFailureInfoLink(rowElement, "additional-link", "stderr", iteration.queue.buildbot.layoutTestStderrURLForIteration(iteration, test.path));

            if (hasTestHistory)
                addFailureInfoLink(rowElement, "test-history-link", "history", testHistory.historyPageURLForTest(test.path));

            content.appendChild(rowElement);
        }

        // Work around bug 80159: -webkit-user-select:none not respected when copying content.
        // We set clipboard data manually, temporarily making non-selectable content hidden
        // to easily get accurate selection text.
        content.oncopy = function(event) {
            var iterator = document.createNodeIterator(
                event.currentTarget,
                NodeFilter.SHOW_ELEMENT,
                {
                    acceptNode: function(element) {
                        if (window.getComputedStyle(element).webkitUserSelect !== "none")
                            return NodeFilter.FILTER_ACCEPT;
                        return NodeFilter.FILTER_SKIP;
                    }
                }
            );

            while ((node = iterator.nextNode()))
                node.style.visibility = "visible";

            event.currentTarget.style.visibility = "hidden";
            event.clipboardData.setData('text', window.getSelection());
            event.currentTarget.style.visibility = "";
            return false;
        }

        return content;
    },

    _presentPopoverForLayoutTestRegressions: function(element, popover, iteration)
    {
        if (iteration.layoutTestResults.regressions)
            var content = this._popoverContentForLayoutTestRegressions(iteration);
        else {
            var content = document.createElement("div");
            content.className = "test-results-popover";

            this._addIterationHeadingToPopover(iteration, content, "layout test failures");
            this._addDividerToPopover(content);

            var loadingIndicator = document.createElement("div");
            loadingIndicator.className = "loading-indicator";
            loadingIndicator.textContent = "Loading\u2026";
            content.appendChild(loadingIndicator);

            iteration.loadLayoutTestResults(function() {
                popover.content = this._popoverContentForLayoutTestRegressions(iteration);
            }.bind(this));
        }
        var rect = Dashboard.Rect.rectFromClientRect(element.getBoundingClientRect());
        popover.content = content;
        popover.present(rect, [Dashboard.RectEdge.MIN_Y, Dashboard.RectEdge.MAX_Y, Dashboard.RectEdge.MAX_X, Dashboard.RectEdge.MIN_X]);
        return true;
    },

    _presentPopoverForGenericTestFailures: function(element, popover, iteration)
    {
        function addResultKind(message, url) {
            var line = document.createElement("a");
            line.className = "failing-test-kind-summary";
            line.href = url;
            line.textContent = message;
            line.target = "_blank";
            content.appendChild(line);            
        }

        var content = document.createElement("div");
        content.className = "test-results-popover";

        this._addIterationHeadingToPopover(iteration, content);
        this._addDividerToPopover(content);

        iteration.failedTestSteps.forEach(function(failedStep) {
            if (failedStep.name === "layout-test")
                addResultKind(this._testStepFailureDescriptionWithCount(failedStep), iteration.queue.buildbot.layoutTestResultsURLForIteration(iteration));
            else
                addResultKind(this._testStepFailureDescriptionWithCount(failedStep), failedStep.URL);
        }, this);

        var rect = Dashboard.Rect.rectFromClientRect(element.getBoundingClientRect());
        popover.content = content;
        popover.present(rect, [Dashboard.RectEdge.MIN_Y, Dashboard.RectEdge.MAX_Y, Dashboard.RectEdge.MAX_X, Dashboard.RectEdge.MIN_X]);
        return true;
    }
};
