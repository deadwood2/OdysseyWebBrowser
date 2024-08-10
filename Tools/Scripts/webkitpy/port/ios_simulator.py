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
from webkitpy.common.version import Version
from webkitpy.port.ios import IOSPort
from webkitpy.xcode.device_type import DeviceType
from webkitpy.xcode.simulated_device import DeviceRequest, SimulatedDeviceManager


_log = logging.getLogger(__name__)


class IOSSimulatorPort(IOSPort):
    port_name = "ios-simulator"

    FUTURE_VERSION = 'future'
    ARCHITECTURES = ['x86_64', 'x86']
    DEFAULT_ARCHITECTURE = 'x86_64'

    DEFAULT_DEVICE_CLASS = 'iPhone SE'
    CUSTOM_DEVICE_CLASSES = ['iPad', 'iPhone 7']
    SDK = 'iphonesimulator'

    def __init__(self, host, port_name, **kwargs):
        super(IOSSimulatorPort, self).__init__(host, port_name, **kwargs)

        optional_device_class = self.get_option('device_class')
        self._device_class = optional_device_class if optional_device_class else self.DEFAULT_DEVICE_CLASS
        _log.debug('IOSSimulatorPort _device_class is %s', self._device_class)

    def _device_for_worker_number_map(self):
        return SimulatedDeviceManager.INITIALIZED_DEVICES

    @staticmethod
    def _version_from_name(name):
        if len(name.split('-')) > 2 and name.split('-')[2].isdigit():
            return Version.from_string(name.split('-')[2])
        return None

    @memoized
    def ios_version(self):
        if self.get_option('version'):
            return Version.from_string(self.get_option('version'))
        return IOSSimulatorPort._version_from_name(self._name) if IOSSimulatorPort._version_from_name(self._name) else self.host.platform.xcode_sdk_version('iphonesimulator')

    @memoized
    def default_child_processes(self):
        def booted_ios_devices_filter(device):
            if not device.platform_device.is_booted_or_booting():
                return False
            return device.platform_device.device_type in DeviceType(software_variant='iOS',
                                                                    software_version=self.ios_version())

        if not self.get_option('dedicated_simulators', False):
            num_booted_sims = len(SimulatedDeviceManager.device_by_filter(booted_ios_devices_filter, host=self.host))
            if num_booted_sims:
                return num_booted_sims
        return SimulatedDeviceManager.max_supported_simulators(self.host)

    def _build_driver_flags(self):
        archs = ['ARCHS=i386'] if self.architecture() == 'x86' else []
        sdk = ['--sdk', 'iphonesimulator']
        return archs + sdk

    def _set_device_class(self, device_class):
        self._device_class = device_class if device_class else self.DEFAULT_DEVICE_CLASS

    def _create_devices(self, device_class):
        self._set_device_class(device_class)
        device_type = DeviceType.from_string(self._device_class, self.ios_version())

        _log.debug('')
        _log.debug('creating devices for {}'.format(device_type))

        request = DeviceRequest(
            device_type,
            use_booted_simulator=not self.get_option('dedicated_simulators', False),
            use_existing_simulator=False,
            allow_incomplete_match=True,
        )
        SimulatedDeviceManager.initialize_devices([request] * self.child_processes(), self.host)

    def clean_up_test_run(self):
        super(IOSSimulatorPort, self).clean_up_test_run()
        _log.debug("clean_up_test_run")

        SimulatedDeviceManager.tear_down(self.host)

    def environment_for_api_tests(self):
        no_prefix = super(IOSSimulatorPort, self).environment_for_api_tests()
        result = {}
        SIMCTL_ENV_PREFIX = 'SIMCTL_CHILD_'
        for value in no_prefix:
            if not value.startswith(SIMCTL_ENV_PREFIX):
                result[SIMCTL_ENV_PREFIX + value] = no_prefix[value]
            else:
                result[value] = no_prefix[value]
        return result

    def setup_environ_for_server(self, server_name=None):
        _log.debug("setup_environ_for_server")
        env = super(IOSSimulatorPort, self).setup_environ_for_server(server_name)
        if server_name == self.driver_name():
            if self.get_option('leaks'):
                env['MallocStackLogging'] = '1'
                env['__XPC_MallocStackLogging'] = '1'
                env['MallocScribble'] = '1'
                env['__XPC_MallocScribble'] = '1'
            if self.get_option('guard_malloc'):
                self._append_value_colon_separated(env, 'DYLD_INSERT_LIBRARIES', '/usr/lib/libgmalloc.dylib')
                self._append_value_colon_separated(env, '__XPC_DYLD_INSERT_LIBRARIES', '/usr/lib/libgmalloc.dylib')
        env['XML_CATALOG_FILES'] = ''  # work around missing /etc/catalog <rdar://problem/4292995>
        return env

    def operating_system(self):
        return 'ios-simulator'

    def check_sys_deps(self):
        target_device_type = DeviceType(software_variant='iOS', software_version=self.ios_version())
        for device in SimulatedDeviceManager.available_devices(self.host):
            if device.platform_device.device_type in target_device_type:
                return super(IOSSimulatorPort, self).check_sys_deps()
        _log.error('No Simulated device matching "{}" defined in Xcode iOS SDK'.format(str(target_device_type)))
        return False

    def reset_preferences(self):
        _log.debug("reset_preferences")
        SimulatedDeviceManager.tear_down(self.host)

    def nm_command(self):
        return self.xcrun_find('nm')

    @property
    @memoized
    def developer_dir(self):
        return self._executive.run_command(['xcode-select', '--print-path']).rstrip()

    def logging_patterns_to_strip(self):
        return []

    def stderr_patterns_to_strip(self):
        return []
