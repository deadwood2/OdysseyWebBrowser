/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Connection.h"

#include "DataReference.h"
#include "ImportanceAssertion.h"
#include "MachMessage.h"
#include "MachPort.h"
#include "MachUtilities.h"
#include <WebCore/AXObjectCache.h>
#include <WebKitSystemInterface.h>
#include <mach/mach_error.h>
#include <mach/vm_map.h>
#include <sys/mman.h>
#include <wtf/RunLoop.h>
#include <wtf/spi/darwin/XPCSPI.h>

#if PLATFORM(IOS)
#include "ProcessAssertion.h"
#include <UIKit/UIAccessibility.h>

#if USE(APPLE_INTERNAL_SDK)
#include <AXRuntime/AXDefines.h>
#include <AXRuntime/AXNotificationConstants.h>
#else
#define kAXPidStatusChangedNotification 0
#endif

#endif

#if PLATFORM(MAC)

#if USE(APPLE_INTERNAL_SDK)
#include <HIServices/AccessibilityPriv.h>
#else
typedef enum {
    AXSuspendStatusRunning = 0,
    AXSuspendStatusSuspended,
} AXSuspendStatus;
#endif

extern "C" AXError _AXUIElementNotifyProcessSuspendStatus(AXSuspendStatus);

#endif // PLATFORM(MAC)

namespace IPC {

static const size_t inlineMessageMaxSize = 4096;

// Message flags.
enum {
    MessageBodyIsOutOfLine = 1 << 0
};
    
// ConnectionTerminationWatchdog does two things:
// 1) It sets a watchdog timer to kill the peered process.
// 2) On iOS, make the process runnable for the duration of the watchdog
//    to ensure it has a chance to terminate cleanly.
class ConnectionTerminationWatchdog {
public:
    static void createConnectionTerminationWatchdog(OSObjectPtr<xpc_connection_t>& xpcConnection, Seconds interval)
    {
        new ConnectionTerminationWatchdog(xpcConnection, interval);
    }
    
private:
    ConnectionTerminationWatchdog(OSObjectPtr<xpc_connection_t>& xpcConnection, Seconds interval)
        : m_xpcConnection(xpcConnection)
        , m_watchdogTimer(RunLoop::main(), this, &ConnectionTerminationWatchdog::watchdogTimerFired)
#if PLATFORM(IOS)
        , m_assertion(std::make_unique<WebKit::ProcessAndUIAssertion>(xpc_connection_get_pid(m_xpcConnection.get()), WebKit::AssertionState::Background))
#endif
    {
        m_watchdogTimer.startOneShot(interval);
    }
    
    void watchdogTimerFired()
    {
        xpc_connection_kill(m_xpcConnection.get(), SIGKILL);
        delete this;
    }

    OSObjectPtr<xpc_connection_t> m_xpcConnection;
    RunLoop::Timer<ConnectionTerminationWatchdog> m_watchdogTimer;
#if PLATFORM(IOS)
    std::unique_ptr<WebKit::ProcessAndUIAssertion> m_assertion;
#endif
};
    
void Connection::platformInvalidate()
{
    if (!m_isConnected) {
        if (m_sendPort) {
            mach_port_deallocate(mach_task_self(), m_sendPort);
            m_sendPort = MACH_PORT_NULL;
        }

        if (m_receivePort) {
            mach_port_unguard(mach_task_self(), m_receivePort, reinterpret_cast<mach_port_context_t>(this));
            mach_port_mod_refs(mach_task_self(), m_receivePort, MACH_PORT_RIGHT_RECEIVE, -1);
            m_receivePort = MACH_PORT_NULL;
        }

        return;
    }

    m_pendingOutgoingMachMessage = nullptr;
    m_isConnected = false;

    ASSERT(m_sendPort);
    ASSERT(m_receivePort);

    // Unregister our ports.
    dispatch_source_cancel(m_sendSource);
    dispatch_release(m_sendSource);
    m_sendSource = nullptr;
    m_sendPort = MACH_PORT_NULL;

    dispatch_source_cancel(m_receiveSource);
    dispatch_release(m_receiveSource);
    m_receiveSource = nullptr;
    m_receivePort = MACH_PORT_NULL;
}
    
void Connection::terminateSoon(Seconds interval)
{
    if (m_xpcConnection)
        ConnectionTerminationWatchdog::createConnectionTerminationWatchdog(m_xpcConnection, interval);
}
    
void Connection::platformInitialize(Identifier identifier)
{
    if (m_isServer) {
        m_receivePort = identifier.port;
        m_sendPort = MACH_PORT_NULL;

        mach_port_guard(mach_task_self(), m_receivePort, reinterpret_cast<mach_port_context_t>(this), true);
    } else {
        m_receivePort = MACH_PORT_NULL;
        m_sendPort = identifier.port;
    }

    m_sendSource = nullptr;
    m_receiveSource = nullptr;

    m_xpcConnection = identifier.xpcConnection;
}

bool Connection::open()
{
    if (m_isServer) {
        ASSERT(m_receivePort);
        ASSERT(!m_sendPort);
        
    } else {
        ASSERT(!m_receivePort);
        ASSERT(m_sendPort);

        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &m_receivePort);
        mach_port_guard(mach_task_self(), m_receivePort, reinterpret_cast<mach_port_context_t>(this), true);

#if PLATFORM(MAC)
        mach_port_set_attributes(mach_task_self(), m_receivePort, MACH_PORT_DENAP_RECEIVER, (mach_port_info_t)0, 0);
#endif

        m_isConnected = true;
        
        // Send the initialize message, which contains a send right for the server to use.
        auto encoder = std::make_unique<Encoder>("IPC", "InitializeConnection", 0);
        encoder->encode(MachPort(m_receivePort, MACH_MSG_TYPE_MAKE_SEND));

        initializeSendSource();

        sendMessage(WTFMove(encoder), { });
    }

    // Change the message queue length for the receive port.
    setMachPortQueueLength(m_receivePort, MACH_PORT_QLIMIT_LARGE);

    RefPtr<Connection> connection(this);
    m_receiveSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, m_receivePort, 0, m_connectionQueue->dispatchQueue());
    dispatch_source_set_event_handler(m_receiveSource, [connection] {
        connection->receiveSourceEventHandler();
    });
    dispatch_source_set_cancel_handler(m_receiveSource, [connection, receivePort = m_receivePort] {
        mach_port_unguard(mach_task_self(), receivePort, reinterpret_cast<mach_port_context_t>(connection.get()));
        mach_port_mod_refs(mach_task_self(), receivePort, MACH_PORT_RIGHT_RECEIVE, -1);
    });

    ref();
    dispatch_async(m_connectionQueue->dispatchQueue(), ^{
        dispatch_resume(m_receiveSource);

        if (m_sendSource)
            dispatch_resume(m_sendSource);

        deref();
    });

    return true;
}

bool Connection::sendMessage(std::unique_ptr<MachMessage> message)
{
    ASSERT(message);
    ASSERT(!m_pendingOutgoingMachMessage);

    // Send the message.
    kern_return_t kr = mach_msg(message->header(), MACH_SEND_MSG | MACH_SEND_TIMEOUT | MACH_SEND_NOTIFY, message->size(), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    switch (kr) {
    case MACH_MSG_SUCCESS:
        // The kernel has already adopted the descriptors.
        message->leakDescriptors();
        return true;

    case MACH_SEND_TIMED_OUT:
        // We timed out, stash away the message for later.
        m_pendingOutgoingMachMessage = WTFMove(message);
        return false;

    case MACH_SEND_INVALID_DEST:
        // The other end has disappeared, we'll get a dead name notification which will cause us to be invalidated.
        return false;

    default:
        WKSetCrashReportApplicationSpecificInformation((CFStringRef)[NSString stringWithFormat:@"Unhandled error code %x, message '%@'", kr, message->messageName()]);
        CRASH();
    }
}

bool Connection::platformCanSendOutgoingMessages() const
{
    return !m_pendingOutgoingMachMessage;
}

bool Connection::sendOutgoingMessage(std::unique_ptr<Encoder> encoder)
{
    ASSERT(!m_pendingOutgoingMachMessage);

    Vector<Attachment> attachments = encoder->releaseAttachments();
    
    size_t numberOfPortDescriptors = 0;
    size_t numberOfOOLMemoryDescriptors = 0;
    for (size_t i = 0; i < attachments.size(); ++i) {
        Attachment::Type type = attachments[i].type();
        if (type == Attachment::MachPortType)
            numberOfPortDescriptors++;
    }
    
    size_t messageSize = MachMessage::messageSize(encoder->bufferSize(), numberOfPortDescriptors, numberOfOOLMemoryDescriptors);

    bool messageBodyIsOOL = false;
    if (messageSize > inlineMessageMaxSize) {
        messageBodyIsOOL = true;

        numberOfOOLMemoryDescriptors++;
        messageSize = MachMessage::messageSize(0, numberOfPortDescriptors, numberOfOOLMemoryDescriptors);
    }

    auto message = MachMessage::create(messageSize);
    message->setMessageName((__bridge CFStringRef)[NSString stringWithFormat:@"%s:%s:", encoder->messageReceiverName().toString().data(), encoder->messageName().toString().data()]);

    bool isComplex = (numberOfPortDescriptors + numberOfOOLMemoryDescriptors) > 0;

    mach_msg_header_t* header = message->header();
    header->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header->msgh_size = messageSize;
    header->msgh_remote_port = m_sendPort;
    header->msgh_local_port = MACH_PORT_NULL;
    header->msgh_id = 0;
    if (messageBodyIsOOL)
        header->msgh_id |= MessageBodyIsOutOfLine;

    uint8_t* messageData;

    if (isComplex) {
        header->msgh_bits |= MACH_MSGH_BITS_COMPLEX;

        mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
        body->msgh_descriptor_count = numberOfPortDescriptors + numberOfOOLMemoryDescriptors;
        uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);

        for (size_t i = 0; i < attachments.size(); ++i) {
            Attachment attachment = attachments[i];

            mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);
            switch (attachment.type()) {
            case Attachment::MachPortType:
                descriptor->port.name = attachment.port();
                descriptor->port.disposition = attachment.disposition();
                descriptor->port.type = MACH_MSG_PORT_DESCRIPTOR;            

                descriptorData += sizeof(mach_msg_port_descriptor_t);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
        }

        if (messageBodyIsOOL) {
            mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);

            descriptor->out_of_line.address = encoder->buffer();
            descriptor->out_of_line.size = encoder->bufferSize();
            descriptor->out_of_line.copy = MACH_MSG_VIRTUAL_COPY;
            descriptor->out_of_line.deallocate = false;
            descriptor->out_of_line.type = MACH_MSG_OOL_DESCRIPTOR;

            descriptorData += sizeof(mach_msg_ool_descriptor_t);
        }

        messageData = descriptorData;
    } else
        messageData = (uint8_t*)(header + 1);

    // Copy the data if it is not being sent out-of-line.
    if (!messageBodyIsOOL)
        memcpy(messageData, encoder->buffer(), encoder->bufferSize());

    ASSERT(m_sendPort);

    return sendMessage(WTFMove(message));
}

void Connection::initializeSendSource()
{
    m_sendSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_SEND, m_sendPort, DISPATCH_MACH_SEND_DEAD | DISPATCH_MACH_SEND_POSSIBLE, m_connectionQueue->dispatchQueue());

    RefPtr<Connection> connection(this);
    dispatch_source_set_event_handler(m_sendSource, [connection] {
        if (!connection->m_sendSource)
            return;

        unsigned long data = dispatch_source_get_data(connection->m_sendSource);

        if (data & DISPATCH_MACH_SEND_DEAD) {
            connection->connectionDidClose();
            return;
        }

        if (data & DISPATCH_MACH_SEND_POSSIBLE) {
            // FIXME: Figure out why we get spurious DISPATCH_MACH_SEND_POSSIBLE events.
            if (connection->m_pendingOutgoingMachMessage)
                connection->sendMessage(WTFMove(connection->m_pendingOutgoingMachMessage));
            connection->sendOutgoingMessages();
            return;
        }
    });

    mach_port_t sendPort = m_sendPort;
    dispatch_source_set_cancel_handler(m_sendSource, ^{
        // Release our send right.
        mach_port_deallocate(mach_task_self(), sendPort);
    });
}

static std::unique_ptr<Decoder> createMessageDecoder(mach_msg_header_t* header)
{
    if (!(header->msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
        // We have a simple message.
        uint8_t* body = reinterpret_cast<uint8_t*>(header + 1);
        size_t bodySize = header->msgh_size - sizeof(mach_msg_header_t);

        return std::make_unique<Decoder>(body, bodySize, nullptr, Vector<Attachment> { });
    }

    bool messageBodyIsOOL = header->msgh_id & MessageBodyIsOutOfLine;

    mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
    mach_msg_size_t numDescriptors = body->msgh_descriptor_count;
    ASSERT(numDescriptors);

    uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);

    // If the message body was sent out-of-line, don't treat the last descriptor
    // as an attachment, since it is really the message body.
    if (messageBodyIsOOL)
        --numDescriptors;

    // Build attachment list
    Vector<Attachment> attachments(numDescriptors);

    for (mach_msg_size_t i = 0; i < numDescriptors; ++i) {
        mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);

        switch (descriptor->type.type) {
        case MACH_MSG_PORT_DESCRIPTOR:
            attachments[numDescriptors - i - 1] = Attachment(descriptor->port.name, descriptor->port.disposition);
            descriptorData += sizeof(mach_msg_port_descriptor_t);
            break;
        default:
            ASSERT(false && "Unhandled descriptor type");
        }
    }

    if (messageBodyIsOOL) {
        mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);
        ASSERT(descriptor->type.type == MACH_MSG_OOL_DESCRIPTOR);

        uint8_t* messageBody = static_cast<uint8_t*>(descriptor->out_of_line.address);
        size_t messageBodySize = descriptor->out_of_line.size;

        auto decoder = std::make_unique<Decoder>(messageBody, messageBodySize, [](const uint8_t* buffer, size_t length) {
            vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(buffer), length);
        }, WTFMove(attachments));

        return decoder;
    }

    uint8_t* messageBody = descriptorData;
    size_t messageBodySize = header->msgh_size - (descriptorData - reinterpret_cast<uint8_t*>(header));

    return std::make_unique<Decoder>(messageBody, messageBodySize, nullptr, attachments);
}

// The receive buffer size should always include the maximum trailer size.
static const size_t receiveBufferSize = inlineMessageMaxSize + MAX_TRAILER_SIZE;
typedef Vector<char, receiveBufferSize> ReceiveBuffer;

static mach_msg_header_t* readFromMachPort(mach_port_t machPort, ReceiveBuffer& buffer)
{
    buffer.resize(receiveBufferSize);

    mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(buffer.data());
    kern_return_t kr = mach_msg(header, MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT, 0, buffer.size(), machPort, 0, MACH_PORT_NULL);
    if (kr == MACH_RCV_TIMED_OUT)
        return 0;

    if (kr == MACH_RCV_TOO_LARGE) {
        // The message was too large, resize the buffer and try again.
        buffer.resize(header->msgh_size + MAX_TRAILER_SIZE);
        header = reinterpret_cast<mach_msg_header_t*>(buffer.data());
        
        kr = mach_msg(header, MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT, 0, buffer.size(), machPort, 0, MACH_PORT_NULL);
        ASSERT(kr != MACH_RCV_TOO_LARGE);
    }

    if (kr != MACH_MSG_SUCCESS) {
#if !ASSERT_DISABLED
        WKSetCrashReportApplicationSpecificInformation((CFStringRef)[NSString stringWithFormat:@"Unhandled error code %x from mach_msg, receive port is %x", kr, machPort]);
#endif
        ASSERT_NOT_REACHED();
        return 0;
    }

    return header;
}

void Connection::receiveSourceEventHandler()
{
    ReceiveBuffer buffer;

    mach_msg_header_t* header = readFromMachPort(m_receivePort, buffer);
    if (!header)
        return;

    switch (header->msgh_id) {
    case MACH_NOTIFY_NO_SENDERS:
        ASSERT(m_isServer);
        if (!m_sendPort)
            connectionDidClose();
        return;

    case MACH_NOTIFY_SEND_ONCE:
        return;

    default:
        break;
    }

    std::unique_ptr<Decoder> decoder = createMessageDecoder(header);
    ASSERT(decoder);

#if PLATFORM(MAC)
    decoder->setImportanceAssertion(std::make_unique<ImportanceAssertion>(header));
#endif

    if (decoder->messageReceiverName() == "IPC" && decoder->messageName() == "InitializeConnection") {
        ASSERT(m_isServer);
        ASSERT(!m_isConnected);
        ASSERT(!m_sendPort);

        MachPort port;
        if (!decoder->decode(port)) {
            // FIXME: Disconnect.
            return;
        }

        m_sendPort = port.port();
        
        if (m_sendPort) {
            mach_port_t previousNotificationPort;
            mach_port_request_notification(mach_task_self(), m_receivePort, MACH_NOTIFY_NO_SENDERS, 0, MACH_PORT_NULL, MACH_MSG_TYPE_MOVE_SEND_ONCE, &previousNotificationPort);

            if (previousNotificationPort != MACH_PORT_NULL)
                mach_port_deallocate(mach_task_self(), previousNotificationPort);

            initializeSendSource();
            dispatch_resume(m_sendSource);
        }

        m_isConnected = true;

        // Send any pending outgoing messages.
        sendOutgoingMessages();
        
        return;
    }

#if !PLATFORM(IOS)
    if (decoder->messageReceiverName() == "IPC" && decoder->messageName() == "SetExceptionPort") {
        if (m_isServer) {
            // Server connections aren't supposed to have their exception ports overriden. Treat this as an invalid message.
            StringReference messageReceiverNameReference = decoder->messageReceiverName();
            String messageReceiverName(String(messageReceiverNameReference.data(), messageReceiverNameReference.size()));
            StringReference messageNameReference = decoder->messageName();
            String messageName(String(messageNameReference.data(), messageNameReference.size()));

            RunLoop::main().dispatch([protectedThis = makeRef(*this), messageReceiverName = WTFMove(messageReceiverName), messageName = WTFMove(messageName)]() mutable {
                protectedThis->dispatchDidReceiveInvalidMessage(messageReceiverName.utf8(), messageName.utf8());
            });
            return;
        }
        MachPort exceptionPort;
        if (!decoder->decode(exceptionPort))
            return;

        setMachExceptionPort(exceptionPort.port());
        return;
    }
#endif

    processIncomingMessage(WTFMove(decoder));
}    

IPC::Connection::Identifier Connection::identifier() const
{
    return Identifier(m_isServer ? m_receivePort : m_sendPort, m_xpcConnection);
}

bool Connection::getAuditToken(audit_token_t& auditToken)
{
    if (!m_xpcConnection)
        return false;
    
    xpc_connection_get_audit_token(m_xpcConnection.get(), &auditToken);
    return true;
}

bool Connection::kill()
{
    if (m_xpcConnection) {
        xpc_connection_kill(m_xpcConnection.get(), SIGKILL);
        return true;
    }

    return false;
}
    
static void AccessibilityProcessSuspendedNotification(bool suspended)
{
#if PLATFORM(MAC)
    _AXUIElementNotifyProcessSuspendStatus(suspended ? AXSuspendStatusSuspended : AXSuspendStatusRunning);
#elif PLATFORM(IOS)
    UIAccessibilityPostNotification(kAXPidStatusChangedNotification, @{ @"pid" : @(getpid()), @"suspended" : @(suspended) });
#else
    UNUSED_PARAM(suspended);
#endif
}
    
void Connection::willSendSyncMessage(OptionSet<SendSyncOption> sendSyncOptions)
{
    if (sendSyncOptions.contains(IPC::SendSyncOption::InformPlatformProcessWillSuspend) && WebCore::AXObjectCache::accessibilityEnabled())
        AccessibilityProcessSuspendedNotification(true);
}

void Connection::didReceiveSyncReply(OptionSet<SendSyncOption> sendSyncOptions)
{
    if (sendSyncOptions.contains(IPC::SendSyncOption::InformPlatformProcessWillSuspend) && WebCore::AXObjectCache::accessibilityEnabled())
        AccessibilityProcessSuspendedNotification(false);
}

pid_t Connection::remoteProcessID() const
{
    if (!m_xpcConnection)
        return 0;

    return xpc_connection_get_pid(m_xpcConnection.get());
}
    
} // namespace IPC
