/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "MessagePort.h"

#include "Document.h"
#include "EventNames.h"
#include "Logging.h"
#include "MessageEvent.h"
#include "MessagePortChannelProvider.h"
#include "MessageWithMessagePorts.h"
#include "WorkerGlobalScope.h"
#include "WorkerThread.h"

namespace WebCore {

static Lock allMessagePortsLock;
static HashMap<MessagePortIdentifier, MessagePort*>& allMessagePorts()
{
    static NeverDestroyed<HashMap<MessagePortIdentifier, MessagePort*>> map;
    return map;
}

void MessagePort::ref() const
{
    ++m_refCount;
}

void MessagePort::deref() const
{
    // This custom deref() function ensures that as long as the lock to allMessagePortsLock is taken, no MessagePort will be destroyed.
    // This allows isExistingMessagePortLocallyReachable and notifyMessageAvailable to easily query the map and manipulate MessagePort instances.

    if (!--m_refCount) {
        Locker<Lock> locker(allMessagePortsLock);

        if (m_refCount)
            return;

        auto iterator = allMessagePorts().find(m_identifier);
        if (iterator != allMessagePorts().end() && iterator->value == this)
            allMessagePorts().remove(iterator);

        delete this;
    }
}

bool MessagePort::isExistingMessagePortLocallyReachable(const MessagePortIdentifier& identifier)
{
    Locker<Lock> locker(allMessagePortsLock);
    auto* port = allMessagePorts().get(identifier);
    return port && port->isLocallyReachable();
}

void MessagePort::notifyMessageAvailable(const MessagePortIdentifier& identifier)
{
    Locker<Lock> locker(allMessagePortsLock);
    if (auto* port = allMessagePorts().get(identifier))
        port->messageAvailable();

}

Ref<MessagePort> MessagePort::create(ScriptExecutionContext& scriptExecutionContext, const MessagePortIdentifier& local, const MessagePortIdentifier& remote)
{
    return adoptRef(*new MessagePort(scriptExecutionContext, local, remote));
}

MessagePort::MessagePort(ScriptExecutionContext& scriptExecutionContext, const MessagePortIdentifier& local, const MessagePortIdentifier& remote)
    : ActiveDOMObject(&scriptExecutionContext)
    , m_identifier(local)
    , m_remoteIdentifier(remote)
{
    LOG(MessagePorts, "Created MessagePort %s (%p) in process %" PRIu64, m_identifier.logString().utf8().data(), this, Process::identifier().toUInt64());

    Locker<Lock> locker(allMessagePortsLock);
    allMessagePorts().set(m_identifier, this);

    m_scriptExecutionContext->createdMessagePort(*this);
    suspendIfNeeded();

    // Don't need to call processMessageWithMessagePortsSoon() here, because the port will not be opened until start() is invoked.
}

MessagePort::~MessagePort()
{
    LOG(MessagePorts, "Destroyed MessagePort %s (%p) in process %" PRIu64, m_identifier.logString().utf8().data(), this, Process::identifier().toUInt64());

    ASSERT(allMessagePortsLock.isLocked());

    if (m_entangled)
        close();

    if (m_scriptExecutionContext)
        m_scriptExecutionContext->destroyedMessagePort(*this);
}

void MessagePort::entangle()
{
    MessagePortChannelProvider::singleton().entangleLocalPortInThisProcessToRemote(m_identifier, m_remoteIdentifier);
}

ExceptionOr<void> MessagePort::postMessage(JSC::ExecState& state, JSC::JSValue messageValue, Vector<JSC::Strong<JSC::JSObject>>&& transfer)
{
    LOG(MessagePorts, "Attempting to post message to port %s (to be received by port %s)", m_identifier.logString().utf8().data(), m_remoteIdentifier.logString().utf8().data());

    registerLocalActivity();

    Vector<RefPtr<MessagePort>> ports;
    auto messageData = SerializedScriptValue::create(state, messageValue, WTFMove(transfer), ports);
    if (messageData.hasException())
        return messageData.releaseException();

    if (!isEntangled())
        return { };
    ASSERT(m_scriptExecutionContext);

    TransferredMessagePortArray transferredPorts;
    // Make sure we aren't connected to any of the passed-in ports.
    if (!ports.isEmpty()) {
        for (auto& port : ports) {
            if (port->identifier() == m_identifier || port->identifier() == m_remoteIdentifier)
                return Exception { DataCloneError };
        }

        auto disentangleResult = MessagePort::disentanglePorts(WTFMove(ports));
        if (disentangleResult.hasException())
            return disentangleResult.releaseException();
        transferredPorts = disentangleResult.releaseReturnValue();
    }

    MessageWithMessagePorts message { messageData.releaseReturnValue(), WTFMove(transferredPorts) };

    LOG(MessagePorts, "Actually posting message to port %s (to be received by port %s)", m_identifier.logString().utf8().data(), m_remoteIdentifier.logString().utf8().data());

    MessagePortChannelProvider::singleton().postMessageToRemote(WTFMove(message), m_remoteIdentifier);
    return { };
}

void MessagePort::disentangle()
{
    ASSERT(m_entangled);
    m_entangled = false;

    registerLocalActivity();

    MessagePortChannelProvider::singleton().messagePortDisentangled(m_identifier);

    // We can't receive any messages or generate any events after this, so remove ourselves from the list of active ports.
    ASSERT(m_scriptExecutionContext);
    m_scriptExecutionContext->destroyedMessagePort(*this);
    m_scriptExecutionContext->willDestroyActiveDOMObject(*this);
    m_scriptExecutionContext->willDestroyDestructionObserver(*this);

    m_scriptExecutionContext = nullptr;
}

void MessagePort::registerLocalActivity()
{
    // Any time certain local operations happen, we dirty our own state to delay GC.
    m_hasHadLocalActivitySinceLastCheck = true;
    m_mightBeEligibleForGC = false;
}

// Invoked to notify us that there are messages available for this port.
// This code may be called from another thread, and so should not call any non-threadsafe APIs (i.e. should not call into the entangled channel or access mutable variables).
void MessagePort::messageAvailable()
{
    // This MessagePort object might be disentangled because the port is being transferred,
    // in which case we'll notify it that messages are available once a new end point is created.
    if (!m_scriptExecutionContext)
        return;

    m_scriptExecutionContext->processMessageWithMessagePortsSoon();
}

void MessagePort::start()
{
    // Do nothing if we've been cloned or closed.
    if (!isEntangled())
        return;

    registerLocalActivity();

    ASSERT(m_scriptExecutionContext);
    if (m_started)
        return;

    m_started = true;
    m_scriptExecutionContext->processMessageWithMessagePortsSoon();
}

void MessagePort::close()
{
    m_mightBeEligibleForGC = true;

    if (m_closed)
        return;
    m_closed = true;

    MessagePortChannelProvider::singleton().messagePortClosed(m_identifier);
    removeAllEventListeners();
}

void MessagePort::contextDestroyed()
{
    ASSERT(m_scriptExecutionContext);

    close();
    m_scriptExecutionContext = nullptr;
}

void MessagePort::dispatchMessages()
{
    // Messages for contexts that are not fully active get dispatched too, but JSAbstractEventListener::handleEvent() doesn't call handlers for these.
    // The HTML5 spec specifies that any messages sent to a document that is not fully active should be dropped, so this behavior is OK.
    ASSERT(started());

    if (!isEntangled())
        return;

    RefPtr<WorkerThread> workerThread;
    if (is<WorkerGlobalScope>(*m_scriptExecutionContext))
        workerThread = &downcast<WorkerGlobalScope>(*m_scriptExecutionContext).thread();

    auto messagesTakenHandler = [this, weakThis = makeWeakPtr(this), workerThread = WTFMove(workerThread)](Vector<MessageWithMessagePorts>&& messages, Function<void()>&& completionCallback) mutable {
        ASSERT(isMainThread());
        auto innerHandler = [this, weakThis = WTFMove(weakThis)](auto&& messages) {
            if (!weakThis)
                return;

            LOG(MessagePorts, "MessagePort %s (%p) dispatching %zu messages", m_identifier.logString().utf8().data(), this, messages.size());

            if (!m_scriptExecutionContext)
                return;

            if (!messages.isEmpty())
                registerLocalActivity();

            ASSERT(m_scriptExecutionContext->isContextThread());

            bool contextIsWorker = is<WorkerGlobalScope>(*m_scriptExecutionContext);
            for (auto& message : messages) {
                // close() in Worker onmessage handler should prevent next message from dispatching.
                if (contextIsWorker && downcast<WorkerGlobalScope>(*m_scriptExecutionContext).isClosing())
                    return;
                auto ports = MessagePort::entanglePorts(*m_scriptExecutionContext, WTFMove(message.transferredPorts));
                dispatchEvent(MessageEvent::create(WTFMove(ports), message.message.releaseNonNull()));
            }
        };

        if (!workerThread) {
            innerHandler(WTFMove(messages));
            completionCallback();
            return;
        }
        workerThread->runLoop().postTaskForMode([innerHandler = WTFMove(innerHandler), messages = WTFMove(messages), completionCallback = WTFMove(completionCallback)](auto&) mutable {
            innerHandler(WTFMove(messages));
            callOnMainThread([completionCallback = WTFMove(completionCallback)] {
                completionCallback();
            });
        }, WorkerRunLoop::defaultMode());
    };

    MessagePortChannelProvider::singleton().takeAllMessagesForPort(m_identifier, WTFMove(messagesTakenHandler));
}

void MessagePort::updateActivity(MessagePortChannelProvider::HasActivity hasActivity)
{
    bool hasHadLocalActivity = m_hasHadLocalActivitySinceLastCheck;
    m_hasHadLocalActivitySinceLastCheck = false;

    if (hasActivity == MessagePortChannelProvider::HasActivity::No && !hasHadLocalActivity)
        m_isRemoteEligibleForGC = true;

    if (hasActivity == MessagePortChannelProvider::HasActivity::Yes)
        m_isRemoteEligibleForGC = false;

    m_isAskingRemoteAboutGC = false;
}

bool MessagePort::hasPendingActivity() const
{
    m_mightBeEligibleForGC = true;

    // If the ScriptExecutionContext has been shut down on this object close()'ed, we can GC.
    if (!m_scriptExecutionContext || m_closed)
        return false;

    // If this object has been idle since the remote port declared itself elgibile for GC, we can GC.
    if (!m_hasHadLocalActivitySinceLastCheck && m_isRemoteEligibleForGC)
        return false;

    // If this MessagePort has no message event handler then the existence of remote activity cannot keep it alive.
    if (!m_hasMessageEventListener)
        return false;

    // If we're not in the middle of asking the remote port about collectability, do so now.
    if (!m_isAskingRemoteAboutGC) {
        RefPtr<WorkerThread> workerThread;
        if (is<WorkerGlobalScope>(*m_scriptExecutionContext))
            workerThread = &downcast<WorkerGlobalScope>(*m_scriptExecutionContext).thread();

        MessagePortChannelProvider::singleton().checkRemotePortForActivity(m_remoteIdentifier, [weakThis = makeWeakPtr(const_cast<MessagePort*>(this)), workerThread = WTFMove(workerThread)](MessagePortChannelProvider::HasActivity hasActivity) mutable {

            ASSERT(isMainThread());
            if (!workerThread) {
                if (weakThis)
                    weakThis->updateActivity(hasActivity);
                return;
            }

            workerThread->runLoop().postTaskForMode([weakThis = WTFMove(weakThis), hasActivity](auto&) mutable {
                if (weakThis)
                    weakThis->updateActivity(hasActivity);
            }, WorkerRunLoop::defaultMode());
        });
        m_isAskingRemoteAboutGC = true;
    }

    // Since we need an answer from the remote object, we have to pretend we have pending activity for now.
    return true;
}

bool MessagePort::isLocallyReachable() const
{
    return !m_mightBeEligibleForGC;
}

MessagePort* MessagePort::locallyEntangledPort() const
{
    // FIXME: As the header describes, this is an optional optimization.
    // Even in the new async model we should be able to get it right.
    return nullptr;
}

ExceptionOr<TransferredMessagePortArray> MessagePort::disentanglePorts(Vector<RefPtr<MessagePort>>&& ports)
{
    if (ports.isEmpty())
        return TransferredMessagePortArray { };

    // Walk the incoming array - if there are any duplicate ports, or null ports or cloned ports, throw an error (per section 8.3.3 of the HTML5 spec).
    HashSet<MessagePort*> portSet;
    for (auto& port : ports) {
        if (!port || !port->m_entangled || !portSet.add(port.get()).isNewEntry)
            return Exception { DataCloneError };
    }

    // Passed-in ports passed validity checks, so we can disentangle them.
    TransferredMessagePortArray portArray;
    portArray.reserveInitialCapacity(ports.size());
    for (auto& port : ports) {
        portArray.uncheckedAppend({ port->identifier(), port->remoteIdentifier() });
        port->disentangle();
    }

    return portArray;
}

Vector<RefPtr<MessagePort>> MessagePort::entanglePorts(ScriptExecutionContext& context, TransferredMessagePortArray&& transferredPorts)
{
    LOG(MessagePorts, "Entangling %zu transferred ports to ScriptExecutionContext %s (%p)", transferredPorts.size(), context.url().string().utf8().data(), &context);

    if (transferredPorts.isEmpty())
        return { };

    Vector<RefPtr<MessagePort>> ports;
    ports.reserveInitialCapacity(transferredPorts.size());
    for (auto& transferredPort : transferredPorts) {
        auto port = MessagePort::create(context, transferredPort.first, transferredPort.second);
        port->entangle();
        ports.uncheckedAppend(WTFMove(port));
    }
    return ports;
}

bool MessagePort::addEventListener(const AtomicString& eventType, Ref<EventListener>&& listener, const AddEventListenerOptions& options)
{
    if (eventType == eventNames().messageEvent) {
        if (listener->isAttribute())
            start();
        m_hasMessageEventListener = true;
        registerLocalActivity();
    }

    return EventTargetWithInlineData::addEventListener(eventType, WTFMove(listener), options);
}

bool MessagePort::removeEventListener(const AtomicString& eventType, EventListener& listener, const ListenerOptions& options)
{
    auto result = EventTargetWithInlineData::removeEventListener(eventType, listener, options);

    if (!hasEventListeners(eventNames().messageEvent))
        m_hasMessageEventListener = false;

    return result;
}

const char* MessagePort::activeDOMObjectName() const
{
    return "MessagePort";
}

bool MessagePort::canSuspendForDocumentSuspension() const
{
    return !hasPendingActivity() || (!m_started || m_closed);
}

} // namespace WebCore
