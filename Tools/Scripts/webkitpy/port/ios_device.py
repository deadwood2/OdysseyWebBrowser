# Copyright (C) 2014-2017 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import logging

from webkitpy.common.memoized import memoized
from webkitpy.common.system.crashlogs import CrashLogs
from webkitpy.port.config import apple_additions
from webkitpy.port.ios import IOSPort

_log = logging.getLogger(__name__)


class IOSDevicePort(IOSPort):
    port_name = 'ios-device'

    ARCHITECTURES = ['armv7', 'armv7s', 'arm64']
    DEFAULT_ARCHITECTURE = 'arm64'
    VERSION_FALLBACK_ORDER = ['ios-7', 'ios-8', 'ios-9', 'ios-10']
    SDK = apple_additions().ios_device_SDK() if apple_additions() else 'iphoneos'
    NO_ON_DEVICE_TESTING = 'On-device testing is not supported on this machine'

    @memoized
    def default_child_processes(self):
        if apple_additions():
            return apple_additions().ios_device_default_child_processes(self)
        return 1

    def using_multiple_devices(self):
        return True

    def _device_for_worker_number_map(self):
        if not apple_additions():
            raise RuntimeError(self.NO_ON_DEVICE_TESTING)
        return apple_additions().ios_device_for_worker_number_map(self)

    def _driver_class(self):
        if apple_additions():
            return apple_additions().ios_device_driver()
        return super(IOSDevicePort, self)._driver_class()

    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        if port_name == cls.port_name:
            iphoneos_sdk_version = host.platform.xcode_sdk_version(cls.SDK)
            if not iphoneos_sdk_version:
                raise Exception("Please install the iOS SDK.")
            major_version_number = iphoneos_sdk_version.split('.')[0]
            port_name = port_name + '-' + major_version_number
        return port_name

    def path_to_crash_logs(self):
        if not apple_additions():
            raise RuntimeError(self.NO_ON_DEVICE_TESTING)
        return apple_additions().ios_crash_log_path()

    def _look_for_all_crash_logs_in_log_dir(self, newer_than):
        log_list = {}
        for device in self._device_for_worker_number_map():
            crash_log = CrashLogs(device, self.path_to_crash_logs(), crash_logs_to_skip=self._crash_logs_to_skip_for_host.get(device, []))
            log_list.update(crash_log.find_all_logs(include_errors=True, newer_than=newer_than))
        return log_list

    def _get_crash_log(self, name, pid, stdout, stderr, newer_than, time_fn=None, sleep_fn=None, wait_for_log=True, target_host=None):
        if target_host:
            return super(IOSDevicePort, self)._get_crash_log(name, pid, stdout, stderr, newer_than, time_fn=time_fn, sleep_fn=sleep_fn, wait_for_log=wait_for_log, target_host=target_host)

        # We need to search every device since one was not specified.
        for device in self._device_for_worker_number_map():
            stderr_out, crashlog = super(IOSDevicePort, self)._get_crash_log(name, pid, stdout, stderr, newer_than, time_fn=time_fn, sleep_fn=sleep_fn, wait_for_log=False, target_host=device)
            if crashlog:
                return (stderr, crashlog)
        return (stderr, None)

    @memoized
    def ios_version(self):
        if self.get_option('version'):
            return self.get_option('version')

        if not apple_additions():
            raise RuntimeError(self.NO_ON_DEVICE_TESTING)

        if not self._device_for_worker_number_map():
            raise RuntimeError('No devices are available')
        version = None
        for device in self._device_for_worker_number_map():
            if not version:
                version = device.platform.os_version
            else:
                if device.platform.os_version != version:
                    raise RuntimeError('Multiple connected devices have different iOS versions')

        return version

    # FIXME: These need device implementations <rdar://problem/30497991>.
    def check_for_leaks(self, process_name, process_pid):
        pass

    # Despite their names, these flags do not actually get passed all the way down to webkit-build.
    def _build_driver_flags(self):
        return ['--sdk', self.SDK] + (['ARCHS=%s' % self.architecture()] if self.architecture() else [])

    def operating_system(self):
        return 'ios-device'

    def _create_devices(self, device_class):
        if not apple_additions():
            raise RuntimeError(self.NO_ON_DEVICE_TESTING)
        if not self._device_for_worker_number_map():
            raise RuntimeError('No devices are available for testing')

        if self.default_child_processes() < self.child_processes():
            raise RuntimeError('Not enought connected devices for {} processes'.format(self.child_processes()))

    def clean_up_test_run(self):
        super(IOSDevicePort, self).clean_up_test_run()
        apple_additions().ios_device_clean_up_test_run(self, self._path_to_driver(), self._build_path())
