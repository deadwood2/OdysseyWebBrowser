# Copyright (C) 2017 Apple Inc. All rights reserved.
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

import time

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.executive_mock import MockExecutive2, ScriptError
from webkitpy.port.ios_device import IOSDevicePort
from webkitpy.port import ios_testcase


class IOSDeviceTest(ios_testcase.IOSTest):
    os_name = 'ios'
    os_version = ''
    port_name = 'ios-device'
    port_maker = IOSDevicePort

    def make_port(self, host=None, port_name=None, options=None, os_name=None, os_version=None, **kwargs):
        port = super(IOSDeviceTest, self).make_port(host=host, port_name=port_name, options=options, os_name=os_name, s_version=os_version, kwargs=kwargs)
        port.set_option('version', '11.0')
        return port

    def test_operating_system(self):
        self.assertEqual('ios-device', self.make_port().operating_system())

    def test_crashlog_path(self):
        port = self.make_port()
        with self.assertRaises(RuntimeError):
            port.path_to_crash_logs()

    def test_spindump(self):
        def logging_run_command(args):
            print args

        port = self.make_port()
        port.host.filesystem.files['/__im_tmp/tmp_0_/test-42-spindump.txt'] = 'Spindump file'
        port.host.executive = MockExecutive2(run_command_fn=logging_run_command)
        expected_stdout = "['/usr/sbin/spindump', 42, 10, 10, '-file', '/__im_tmp/tmp_0_/test-42-spindump.txt']\n"
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42], expected_stdout=expected_stdout)
        self.assertEqual(port.host.filesystem.files['/mock-build/layout-test-results/test-42-spindump.txt'], 'Spindump file')
        self.assertIsNone(port.host.filesystem.files['/__im_tmp/tmp_0_/test-42-spindump.txt'])

    def test_sample_process(self):
        def logging_run_command(args):
            if args[0] == '/usr/sbin/spindump':
                return 1
            print args
            return 0

        port = self.make_port()
        port.host.filesystem.files['/__im_tmp/tmp_0_/test-42-sample.txt'] = 'Sample file'
        port.host.executive = MockExecutive2(run_command_fn=logging_run_command)
        expected_stdout = "['/usr/bin/sample', 42, 10, 10, '-file', '/__im_tmp/tmp_0_/test-42-sample.txt']\n"
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42], expected_stdout=expected_stdout)
        self.assertEqual(port.host.filesystem.files['/mock-build/layout-test-results/test-42-sample.txt'], 'Sample file')
        self.assertIsNone(port.host.filesystem.files['/__im_tmp/tmp_0_/test-42-sample.txt'])

    def test_sample_process_exception(self):
        def throwing_run_command(args):
            if args[0] == '/usr/sbin/spindump':
                return 1
            raise ScriptError('MOCK script error')

        port = self.make_port()
        port.host.executive = MockExecutive2(run_command_fn=throwing_run_command)
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42])

    def test_get_crash_log(self):
        port = self.make_port(port_name=self.port_name)
        with self.assertRaises(RuntimeError):
            port._get_crash_log('DumpRenderTree', 1234, None, None, time.time(), wait_for_log=False)
