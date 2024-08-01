/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorController_h
#define InspectorController_h

#include "InspectorOverlay.h"
#include "PageScriptDebugServer.h"
#include <inspector/InspectorAgentRegistry.h>
#include <inspector/InspectorEnvironment.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace Inspector {
class BackendDispatcher;
class FrontendChannel;
class FrontendRouter;
class InspectorAgent;

namespace Protocol {
namespace OverlayTypes {
class NodeHighlightData;
}
}
}

namespace WebCore {

class DOMWrapperWorld;
class Frame;
class GraphicsContext;
class InspectorClient;
class InspectorDOMAgent;
class InspectorFrontendClient;
class InspectorInstrumentation;
class InspectorPageAgent;
class InspectorTimelineAgent;
class InstrumentingAgents;
class Node;
class Page;
class WebInjectedScriptManager;

class InspectorController final : public Inspector::InspectorEnvironment {
    WTF_MAKE_NONCOPYABLE(InspectorController);
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorController(Page&, InspectorClient*);
    virtual ~InspectorController();

    void inspectedPageDestroyed();

    bool enabled() const;
    Page& inspectedPage() const;

    WEBCORE_EXPORT void show();

    WEBCORE_EXPORT void setInspectorFrontendClient(InspectorFrontendClient*);
    unsigned inspectionLevel() const;
    void didClearWindowObjectInWorld(Frame&, DOMWrapperWorld&);

    WEBCORE_EXPORT void dispatchMessageFromFrontend(const String& message);

    bool hasLocalFrontend() const;
    bool hasRemoteFrontend() const;

    WEBCORE_EXPORT void connectFrontend(Inspector::FrontendChannel*, bool isAutomaticInspection = false);
    WEBCORE_EXPORT void disconnectFrontend(Inspector::FrontendChannel*);
    WEBCORE_EXPORT void disconnectAllFrontends();
    void setProcessId(long);

    void inspect(Node*);
    WEBCORE_EXPORT void drawHighlight(GraphicsContext&) const;
    WEBCORE_EXPORT void getHighlight(Highlight&, InspectorOverlay::CoordinateSystem) const;
    void hideHighlight();
    Node* highlightedNode() const;

    void setIndicating(bool);

    WEBCORE_EXPORT Ref<Inspector::Protocol::Array<Inspector::Protocol::OverlayTypes::NodeHighlightData>> buildObjectForHighlightedNodes() const;

    bool isUnderTest() const { return m_isUnderTest; }
    void setIsUnderTest(bool isUnderTest) { m_isUnderTest = isUnderTest; }
    WEBCORE_EXPORT void evaluateForTestInFrontend(const String& script);

    WEBCORE_EXPORT bool legacyProfilerEnabled() const;
    WEBCORE_EXPORT void setLegacyProfilerEnabled(bool);

    InspectorClient* inspectorClient() const { return m_inspectorClient; }
    InspectorPageAgent* pageAgent() const { return m_pageAgent; }

    virtual bool developerExtrasEnabled() const override;
    virtual bool canAccessInspectedScriptState(JSC::ExecState*) const override;
    virtual Inspector::InspectorFunctionCallHandler functionCallHandler() const override;
    virtual Inspector::InspectorEvaluateHandler evaluateHandler() const override;
    virtual void frontendInitialized() override;
    virtual Ref<WTF::Stopwatch> executionStopwatch() override;
    virtual PageScriptDebugServer& scriptDebugServer() override;
    virtual JSC::VM& vm() override;

    WEBCORE_EXPORT void didComposite(Frame&);

private:
    friend class InspectorInstrumentation;

    Ref<InstrumentingAgents> m_instrumentingAgents;
    std::unique_ptr<WebInjectedScriptManager> m_injectedScriptManager;
    Ref<Inspector::FrontendRouter> m_frontendRouter;
    Ref<Inspector::BackendDispatcher> m_backendDispatcher;
    std::unique_ptr<InspectorOverlay> m_overlay;
    Ref<WTF::Stopwatch> m_executionStopwatch;
    PageScriptDebugServer m_scriptDebugServer;
    Inspector::AgentRegistry m_agents;

    Page& m_page;
    InspectorClient* m_inspectorClient;
    InspectorFrontendClient* m_inspectorFrontendClient { nullptr };

    Inspector::InspectorAgent* m_inspectorAgent { nullptr };
    InspectorDOMAgent* m_domAgent { nullptr };
    InspectorPageAgent* m_pageAgent { nullptr };
    InspectorTimelineAgent* m_timelineAgent { nullptr };

    bool m_isUnderTest { false };
    bool m_isAutomaticInspection { false };
    bool m_legacyProfilerEnabled { false };
};

} // namespace WebCore

#endif // !defined(InspectorController_h)
