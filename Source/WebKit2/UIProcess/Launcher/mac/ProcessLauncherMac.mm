/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#import "config.h"
#import "ProcessLauncher.h"

#import <WebCore/ServersSPI.h>
#import <WebCore/SoftLinking.h>
#import <WebCore/WebCoreNSStringExtras.h>
#import <crt_externs.h>
#import <mach-o/dyld.h>
#import <mach/machine.h>
#import <spawn.h>
#import <sys/param.h>
#import <sys/stat.h>
#import <wtf/RunLoop.h>
#import <wtf/Threading.h>
#import <wtf/spi/darwin/XPCSPI.h>
#import <wtf/spi/cf/CFBundleSPI.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#import "CodeSigning.h"
#endif

namespace WebKit {

static const char* serviceName(const ProcessLauncher::LaunchOptions& launchOptions)
{
    switch (launchOptions.processType) {
    case ProcessLauncher::ProcessType::Web:
        return "com.apple.WebKit.WebContent";
    case ProcessLauncher::ProcessType::Network:
        return "com.apple.WebKit.Networking";
#if ENABLE(DATABASE_PROCESS)
    case ProcessLauncher::ProcessType::Database:
        return "com.apple.WebKit.Databases";
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case ProcessLauncher::ProcessType::Plugin32:
        return "com.apple.WebKit.Plugin.32";
    case ProcessLauncher::ProcessType::Plugin64:
        return "com.apple.WebKit.Plugin.64";
#endif
    }
}
    
static bool shouldLeakBoost(const ProcessLauncher::LaunchOptions& launchOptions)
{
#if PLATFORM(IOS)
    // On iOS, leak a boost onto all child processes
    UNUSED_PARAM(launchOptions);
    return true;
#else
    // On Mac, leak a boost onto the NetworkProcess.
    return launchOptions.processType == ProcessLauncher::ProcessType::Network;
#endif
}

static NSString *systemDirectoryPath()
{
    static NSString *path = [^{
#if PLATFORM(IOS_SIMULATOR)
        char *simulatorRoot = getenv("SIMULATOR_ROOT");
        return simulatorRoot ? [NSString stringWithFormat:@"%s/System/", simulatorRoot] : @"/System/";
#else
        return @"/System/";
#endif
    }() copy];

    return path;
}

void ProcessLauncher::launchProcess()
{
    ASSERT(!m_xpcConnection);

    m_xpcConnection = adoptOSObject(xpc_connection_create(serviceName(m_launchOptions), dispatch_get_main_queue()));

    uuid_t uuid;
    uuid_generate(uuid);
    xpc_connection_set_oneshot_instance(m_xpcConnection.get(), uuid);

    // Inherit UI process localization. It can be different from child process default localization:
    // 1. When the application and system frameworks simply have different localized resources available, we should match the application.
    // 1.1. An important case is WebKitTestRunner, where we should use English localizations for all system frameworks.
    // 2. When AppleLanguages is passed as command line argument for UI process, or set in its preferences, we should respect it in child processes.
    auto initializationMessage = adoptOSObject(xpc_dictionary_create(nullptr, nullptr, 0));
    _CFBundleSetupXPCBootstrap(initializationMessage.get());
#if PLATFORM(IOS)
    // Clients that set these environment variables explicitly do not have the values automatically forwarded by libxpc.
    auto containerEnvironmentVariables = adoptOSObject(xpc_dictionary_create(nullptr, nullptr, 0));
    if (const char* environmentHOME = getenv("HOME"))
        xpc_dictionary_set_string(containerEnvironmentVariables.get(), "HOME", environmentHOME);
    if (const char* environmentCFFIXED_USER_HOME = getenv("CFFIXED_USER_HOME"))
        xpc_dictionary_set_string(containerEnvironmentVariables.get(), "CFFIXED_USER_HOME", environmentCFFIXED_USER_HOME);
    if (const char* environmentTMPDIR = getenv("TMPDIR"))
        xpc_dictionary_set_string(containerEnvironmentVariables.get(), "TMPDIR", environmentTMPDIR);
    xpc_dictionary_set_value(initializationMessage.get(), "ContainerEnvironmentVariables", containerEnvironmentVariables.get());
#endif

    auto languagesIterator = m_launchOptions.extraInitializationData.find("OverrideLanguages");
    if (languagesIterator != m_launchOptions.extraInitializationData.end()) {
        auto languages = adoptOSObject(xpc_array_create(nullptr, 0));
        Vector<String> languageVector;
        languagesIterator->value.split(",", false, languageVector);
        for (auto& language : languageVector)
            xpc_array_set_string(languages.get(), XPC_ARRAY_APPEND, language.utf8().data());
        xpc_dictionary_set_value(initializationMessage.get(), "OverrideLanguages", languages.get());
    }

    xpc_connection_set_bootstrap(m_xpcConnection.get(), initializationMessage.get());

    if (shouldLeakBoost(m_launchOptions)) {
        auto preBootstrapMessage = adoptOSObject(xpc_dictionary_create(nullptr, nullptr, 0));
        xpc_dictionary_set_string(preBootstrapMessage.get(), "message-name", "pre-bootstrap");
        xpc_connection_send_message(m_xpcConnection.get(), preBootstrapMessage.get());
    }
    
    // Create the listening port.
    mach_port_t listeningPort;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listeningPort);
    
    // Insert a send right so we can send to it.
    mach_port_insert_right(mach_task_self(), listeningPort, listeningPort, MACH_MSG_TYPE_MAKE_SEND);

    mach_port_t previousNotificationPort;
    mach_port_request_notification(mach_task_self(), listeningPort, MACH_NOTIFY_NO_SENDERS, 0, listeningPort, MACH_MSG_TYPE_MAKE_SEND_ONCE, &previousNotificationPort);
    ASSERT(!previousNotificationPort);

    String clientIdentifier;
#if PLATFORM(MAC)
    clientIdentifier = codeSigningIdentifierForCurrentProcess();
#endif
    if (clientIdentifier.isNull())
        clientIdentifier = [[NSBundle mainBundle] bundleIdentifier];

    // FIXME: Switch to xpc_connection_set_bootstrap once it's available everywhere we need.
    auto bootstrapMessage = adoptOSObject(xpc_dictionary_create(nullptr, nullptr, 0));
    xpc_dictionary_set_string(bootstrapMessage.get(), "message-name", "bootstrap");

    xpc_dictionary_set_mach_send(bootstrapMessage.get(), "server-port", listeningPort);
    mach_port_deallocate(mach_task_self(), listeningPort);

    xpc_dictionary_set_string(bootstrapMessage.get(), "client-identifier", !clientIdentifier.isEmpty() ? clientIdentifier.utf8().data() : *_NSGetProgname());
    xpc_dictionary_set_string(bootstrapMessage.get(), "ui-process-name", [[[NSProcessInfo processInfo] processName] UTF8String]);

    bool isWebKitDevelopmentBuild = ![[[[NSBundle bundleWithIdentifier:@"com.apple.WebKit"] bundlePath] stringByDeletingLastPathComponent] hasPrefix:systemDirectoryPath()];
    if (isWebKitDevelopmentBuild) {
        xpc_dictionary_set_fd(bootstrapMessage.get(), "stdout", STDOUT_FILENO);
        xpc_dictionary_set_fd(bootstrapMessage.get(), "stderr", STDERR_FILENO);
    }

    auto extraInitializationData = adoptOSObject(xpc_dictionary_create(nullptr, nullptr, 0));

    for (const auto& keyValuePair : m_launchOptions.extraInitializationData)
        xpc_dictionary_set_string(extraInitializationData.get(), keyValuePair.key.utf8().data(), keyValuePair.value.utf8().data());

    xpc_dictionary_set_value(bootstrapMessage.get(), "extra-initialization-data", extraInitializationData.get());

    auto weakProcessLauncher = m_weakPtrFactory.createWeakPtr();
    xpc_connection_set_event_handler(m_xpcConnection.get(), [weakProcessLauncher, listeningPort](xpc_object_t event) {
        ASSERT(xpc_get_type(event) == XPC_TYPE_ERROR);

        auto processLauncher = weakProcessLauncher.get();
        if (!processLauncher)
            return;

        if (!processLauncher->isLaunching())
            return;

        // We failed to launch. Release the send right.
        mach_port_deallocate(mach_task_self(), listeningPort);

        // And the receive right.
        mach_port_mod_refs(mach_task_self(), listeningPort, MACH_PORT_RIGHT_RECEIVE, -1);

        processLauncher->m_xpcConnection = nullptr;

        processLauncher->didFinishLaunchingProcess(0, IPC::Connection::Identifier());
    });

    xpc_connection_resume(m_xpcConnection.get());

    ref();
    xpc_connection_send_message_with_reply(m_xpcConnection.get(), bootstrapMessage.get(), dispatch_get_main_queue(), ^(xpc_object_t reply) {
        // Errors are handled in the event handler.
        if (xpc_get_type(reply) != XPC_TYPE_ERROR) {
            ASSERT(xpc_get_type(reply) == XPC_TYPE_DICTIONARY);
            ASSERT(!strcmp(xpc_dictionary_get_string(reply, "message-name"), "process-finished-launching"));

            if (!m_xpcConnection) {
                // The process was terminated.
                didFinishLaunchingProcess(0, IPC::Connection::Identifier());
                return;
            }

            // The process has finished launching, grab the pid from the connection.
            pid_t processIdentifier = xpc_connection_get_pid(m_xpcConnection.get());

            didFinishLaunchingProcess(processIdentifier, IPC::Connection::Identifier(listeningPort, m_xpcConnection));
            m_xpcConnection = nullptr;
        }

        deref();
    });
}

void ProcessLauncher::terminateProcess()
{
    if (m_isLaunching) {
        invalidate();
        return;
    }

    if (!m_processIdentifier)
        return;
    
    kill(m_processIdentifier, SIGKILL);
    m_processIdentifier = 0;
}
    
void ProcessLauncher::platformInvalidate()
{
    if (!m_xpcConnection)
        return;

    xpc_connection_cancel(m_xpcConnection.get());
    xpc_connection_kill(m_xpcConnection.get(), SIGKILL);
    m_xpcConnection = nullptr;
}

} // namespace WebKit
