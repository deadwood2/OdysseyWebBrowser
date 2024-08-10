# Copyright (C) 2018 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import logging
import os

from webkitpy.api_tests.runner import Runner
from webkitpy.common.system.executive import ScriptError
from webkitpy.xcode.device_type import DeviceType
from webkitpy.xcode.simulated_device import DeviceRequest, SimulatedDeviceManager

_log = logging.getLogger(__name__)


class Manager(object):
    """A class for managing running API and WTF tests
    """

    SUCCESS = 0
    FAILED_BUILD_CHECK = 1
    FAILED_COLLECT_TESTS = 2
    FAILED_TESTS = 3

    def __init__(self, port, options, stream):
        self._port = port
        self.host = port.host
        self._options = options
        self._stream = stream

    @staticmethod
    def _test_list_from_output(output, prefix=''):
        result = []
        current_test_suite = None
        for line in output.split('\n'):
            striped_line = line.lstrip().rstrip()
            if not striped_line:
                continue

            if striped_line[-1] == '.':
                current_test_suite = striped_line[:-1]
            else:
                striped_line = striped_line.lstrip()
                if ' ' in striped_line:
                    continue
                val = '{}{}.{}'.format(prefix, current_test_suite, striped_line)
                if val not in result:
                    result.append(val)
        return result

    @staticmethod
    def _find_test_subset(superset, arg_filter):
        result = []
        for arg in arg_filter:
            split_arg = arg.split('.')
            for test in superset:
                # Might match <binary>.<suite>.<test> or just <suite>.<test>
                split_test = test.split('.')
                if len(split_arg) == 1:
                    if test not in result and (arg == split_test[0] or arg == split_test[1]):
                        result.append(test)
                elif len(split_arg) == 2:
                    if test not in result and (split_arg == split_test[0:2] or split_arg == split_test[1:3]):
                        result.append(test)
                else:
                    if arg == test and test not in result:
                        result.append(test)
        return result

    def _collect_tests(self, args):
        available_tests = []
        for binary in self._port.path_to_api_test_binaries():
            stripped_name = os.path.splitext(os.path.basename(binary))[0]
            try:
                output = self.host.executive.run_command(
                    Runner.command_for_port(self._port, [binary, '--gtest_list_tests']),
                    env=self._port.environment_for_api_tests())
                available_tests += Manager._test_list_from_output(output, '{}.'.format(stripped_name))
            except ScriptError:
                _log.error('Failed to list {} tests'.format(stripped_name))
                raise

        if len(args) == 0:
            return sorted(available_tests)
        return sorted(Manager._find_test_subset(available_tests, args))

    @staticmethod
    def _print_test_result(stream, test_name, output):
        stream.writeln('    {}'.format(test_name))
        has_output = False
        for line in output.splitlines():
            stream.writeln('        {}'.format(line))
            has_output = True
        if has_output:
            stream.writeln('')
        return not has_output

    def _print_tests_result_with_status(self, status, runner):
        mapping = runner.result_map_by_status(status)
        if mapping:
            self._stream.writeln(runner.NAME_FOR_STATUS[status])
            self._stream.writeln('')
            need_newline = False
            for test, output in mapping.iteritems():
                need_newline = Manager._print_test_result(self._stream, test, output)
            if need_newline:
                self._stream.writeln('')

    def _initialize_devices(self):
        if 'simulator' in self._port.port_name:
            SimulatedDeviceManager.initialize_devices(DeviceRequest(DeviceType.from_string(self._port.DEFAULT_DEVICE_CLASS), allow_incomplete_match=True), self.host, simulator_ui=False)
        elif 'device' in self._port.port_name:
            raise RuntimeError('Running api tests on {} is not supported'.format(self._port.port_name))

    def run(self, args):
        self._stream.write_update('Checking build ...')
        if not self._port.check_api_test_build():
            _log.error('Build check failed')
            return Manager.FAILED_BUILD_CHECK

        self._initialize_devices()

        self._stream.write_update('Collecting tests ...')
        try:
            test_names = self._collect_tests(args)
        except ScriptError:
            self._stream.writeln('Failed to collect tests')
            return Manager.FAILED_COLLECT_TESTS
        self._stream.write_update('Found {} tests'.format(len(test_names)))
        if len(test_names) == 0:
            self._stream.writeln('No tests found')
            return Manager.FAILED_COLLECT_TESTS

        if self._port.get_option('dump'):
            for test in test_names:
                self._stream.writeln(test)
            return Manager.SUCCESS

        try:
            _log.info('Running tests')
            runner = Runner(self._port, self._stream)
            runner.run(test_names, int(self._options.child_processes) if self._options.child_processes else self._port.default_child_processes())
        except KeyboardInterrupt:
            # If we receive a KeyboardInterrupt, print results.
            self._stream.writeln('')

        successful = runner.result_map_by_status(runner.STATUS_PASSED)
        disabled = len(runner.result_map_by_status(runner.STATUS_DISABLED))
        _log.info('Ran {} tests of {} with {} successful'.format(len(runner.results) - disabled, len(test_names), len(successful)))

        self._stream.writeln('------------------------------')
        if len(successful) + disabled == len(test_names):
            self._stream.writeln('All tests successfully passed!')
            return Manager.SUCCESS

        self._stream.writeln('Test suite failed')
        self._stream.writeln('')

        skipped = []
        for test in test_names:
            if test not in runner.results:
                skipped.append(test)
        if skipped:
            self._stream.writeln('Skipped {} tests'.format(len(skipped)))
            self._stream.writeln('')
            if self._options.verbose:
                for test in skipped:
                    self._stream.writeln('    {}'.format(test))

        self._print_tests_result_with_status(runner.STATUS_FAILED, runner)
        self._print_tests_result_with_status(runner.STATUS_CRASHED, runner)
        self._print_tests_result_with_status(runner.STATUS_TIMEOUT, runner)

        return Manager.FAILED_TESTS
