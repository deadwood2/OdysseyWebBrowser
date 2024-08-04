# Copyright (C) 2014-2016 Apple Inc. All rights reserved.
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

from webkitpy.port import port_testcase
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.mocktool import MockOptions
from webkitpy.common.system.executive_mock import MockExecutive, MockExecutive2, MockProcess, ScriptError
from webkitpy.common.system.systemhost_mock import MockSystemHost


class DarwinTest(port_testcase.PortTestCase):

    def assert_skipped_file_search_paths(self, port_name, expected_paths, use_webkit2=False):
        port = self.make_port(port_name=port_name, options=MockOptions(webkit_test_runner=use_webkit2))
        self.assertEqual(port._skipped_file_search_paths(), expected_paths)

    def test_default_timeout_ms(self):
        super(DarwinTest, self).test_default_timeout_ms()
        self.assertEqual(self.make_port(options=MockOptions(guard_malloc=True)).default_timeout_ms(), 350000)

    def assert_name(self, port_name, os_version_string, expected):
        host = MockSystemHost(os_name=self.os_name, os_version=os_version_string)
        port = self.make_port(host=host, port_name=port_name)
        self.assertEqual(expected, port.name())

    def test_show_results_html_file(self):
        port = self.make_port()
        # Delay setting a should_log executive to avoid logging from MacPort.__init__.
        port._executive = MockExecutive(should_log=True)
        expected_logs = "MOCK popen: ['Tools/Scripts/run-safari', '--release', '--no-saved-state', '-NSOpen', 'test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_logs=expected_logs)

    def test_helper_starts(self):
        host = MockSystemHost(MockExecutive())
        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        host.executive._proc = MockProcess('ready\n')
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

        # make sure trying to stop the helper twice is safe.
        port.stop_helper()

    def test_helper_fails_to_start(self):
        host = MockSystemHost(MockExecutive())
        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

    def test_helper_fails_to_stop(self):
        host = MockSystemHost(MockExecutive())
        host.executive._proc = MockProcess()

        def bad_waiter():
            raise IOError('failed to wait')
        host.executive._proc.wait = bad_waiter

        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

    def test_spindump(self):

        def logging_run_command(args):
            print args

        port = self.make_port()
        port._executive = MockExecutive2(run_command_fn=logging_run_command)
        expected_stdout = "['/usr/bin/sudo', '-n', '/usr/sbin/spindump', 42, 10, 10, '-file', '/mock-build/layout-test-results/test-42-spindump.txt']\n"
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42], expected_stdout=expected_stdout)

    def test_sample_process(self):

        def logging_run_command(args):
            if args[0] == '/usr/bin/sudo':
                return 1
            print args
            return 0

        port = self.make_port()
        port._executive = MockExecutive2(run_command_fn=logging_run_command)
        expected_stdout = "['/usr/bin/sample', 42, 10, 10, '-file', '/mock-build/layout-test-results/test-42-sample.txt']\n"
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42], expected_stdout=expected_stdout)

    def test_sample_process_exception(self):
        def throwing_run_command(args):
            if args[0] == '/usr/bin/sudo':
                return 1
            raise ScriptError("MOCK script error")

        port = self.make_port()
        port._executive = MockExecutive2(run_command_fn=throwing_run_command)
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42])
