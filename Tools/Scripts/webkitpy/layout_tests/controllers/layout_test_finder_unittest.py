# Copyright (c) 2015, Canon Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/gcor other materials provided with the distribution.
# 3.  Neither the name of Canon Inc. nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import optparse
import unittest

from collections import OrderedDict

from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.layout_tests.controllers.layout_test_finder import LayoutTestFinder, Port, _is_reference_html_file, _supported_test_extensions
from webkitpy.port.test import add_unit_tests_to_mock_filesystem, TestPort


class MockLayoutTestFinder(LayoutTestFinder):
    def _real_tests(self, paths):
        return [path for path in paths if path.endswith('.html')]


class LayoutTestFinderTests(unittest.TestCase):
    def make_finder(self):
        host = MockHost(create_stub_repository_files=True)
        add_unit_tests_to_mock_filesystem(host.filesystem)
        port = TestPort(host)
        return LayoutTestFinder(port, None)

    def touched_files(self, touched_files, fs=None):
        host = MockHost()
        if fs:
            host.filesystem = fs
        else:
            fs = host.filesystem
        port = TestPort(host)
        return (fs, MockLayoutTestFinder(port, optparse.Values({'skipped': 'always', 'skip_failing_tests': False, 'http': True})).find_touched_tests(touched_files))

    def test_supported_test_extensions(self):
        self.assertEqual(_supported_test_extensions & Port._supported_reference_extensions, Port._supported_reference_extensions)

    def test_is_reference_html_file(self):
        filesystem = MockFileSystem()
        self.assertTrue(_is_reference_html_file(filesystem, '', 'foo-expected.html'))
        self.assertTrue(_is_reference_html_file(filesystem, '', 'foo-expected-mismatch.xml'))
        self.assertTrue(_is_reference_html_file(filesystem, '', 'foo-ref.xhtml'))
        self.assertTrue(_is_reference_html_file(filesystem, '', 'foo-notref.svg'))
        self.assertFalse(_is_reference_html_file(filesystem, '', 'foo.html'))
        self.assertFalse(_is_reference_html_file(filesystem, '', 'foo-expected.txt'))
        self.assertFalse(_is_reference_html_file(filesystem, '', 'foo-expected.shtml'))
        self.assertFalse(_is_reference_html_file(filesystem, '', 'foo-expected.php'))
        self.assertFalse(_is_reference_html_file(filesystem, '', 'foo-expected.mht'))

    def test_find_no_paths_specified(self):
        finder = self.make_finder()
        tests = finder.find_tests_by_path([])
        self.assertNotEqual(len(tests), 0)

    def test_find_one_test(self):
        finder = self.make_finder()
        tests = finder.find_tests_by_path(['failures/expected/image.html'])
        self.assertEqual(len(tests), 1)

    def test_find_glob(self):
        finder = self.make_finder()
        tests = finder.find_tests_by_path(['failures/expected/im*'])
        self.assertEqual(len(tests), 2)

    def test_find_with_skipped_directories(self):
        finder = self.make_finder()
        tests = finder.find_tests_by_path(['userscripts'])
        self.assertNotIn('userscripts/resources/iframe.html', tests)

    def test_find_with_skipped_directories_2(self):
        finder = self.make_finder()
        tests = finder.find_tests_by_path(['userscripts/resources'])
        self.assertEqual(tests, [])

    def test_is_test_file(self):
        finder = self.make_finder()
        self.assertTrue(finder._is_test_file(finder._filesystem, '', 'foo.html'))
        self.assertTrue(finder._is_test_file(finder._filesystem, '', 'foo.shtml'))
        self.assertTrue(finder._is_test_file(finder._filesystem, '', 'foo.svg'))
        self.assertTrue(finder._is_test_file(finder._filesystem, '', 'test-ref-test.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo.png'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected.svg'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected.xht'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected-mismatch.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected-mismatch.svg'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-expected-mismatch.xhtml'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-ref.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-notref.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-notref.xht'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'foo-ref.xhtml'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'ref-foo.html'))
        self.assertFalse(finder._is_test_file(finder._filesystem, '', 'notref-foo.xhr'))

    def test_is_w3c_resource_file(self):
        finder = self.make_finder()

        finder._filesystem.write_text_file(finder._port.layout_tests_dir() + "/imported/w3c/resources/resource-files.json", """
{"directories": [
"web-platform-tests/common",
"web-platform-tests/dom/nodes/Document-createElement-namespace-tests",
"web-platform-tests/fonts",
"web-platform-tests/html/browsers/browsing-the-web/navigating-across-documents/source/support",
"web-platform-tests/html/browsers/browsing-the-web/unloading-documents/support",
"web-platform-tests/html/browsers/history/the-history-interface/non-automated",
"web-platform-tests/html/browsers/history/the-location-interface/non-automated",
"web-platform-tests/images",
"web-platform-tests/service-workers",
"web-platform-tests/tools"
], "files": [
"web-platform-tests/XMLHttpRequest/xmlhttprequest-sync-block-defer-scripts-subframe.html",
"web-platform-tests/XMLHttpRequest/xmlhttprequest-sync-not-hang-scriptloader-subframe.html"
]}""")
        self.assertFalse(finder._is_w3c_resource_file(finder._filesystem, finder._port.layout_tests_dir() + "/imported/w3", "resource_file.html"))
        self.assertFalse(finder._is_w3c_resource_file(finder._filesystem, finder._port.layout_tests_dir() + "/imported/w3c", "resource_file.html"))
        self.assertFalse(finder._is_w3c_resource_file(finder._filesystem, finder._port.layout_tests_dir() + "/imported/w3c/web-platform-tests/XMLHttpRequest", "xmlhttprequest-sync-block-defer-scripts-subframe.html.html"))
        self.assertTrue(finder._is_w3c_resource_file(finder._filesystem, finder._port.layout_tests_dir() + "/imported/w3c/web-platform-tests/XMLHttpRequest", "xmlhttprequest-sync-block-defer-scripts-subframe.html"))
        self.assertTrue(finder._is_w3c_resource_file(finder._filesystem, finder._port.layout_tests_dir() + "/imported/w3c/web-platform-tests/dom/nodes/Document-createElement-namespace-tests", "test.html"))

    def test_touched_test(self):
        paths = ['LayoutTests/test.html', 'LayoutTests/test', 'test2.html', 'Source/test1.html']
        fs, touched_tests = self.touched_files(paths)
        self.assertEqual(touched_tests, ['test.html'])

    def test_expected_touched_test(self):
        paths = ['LayoutTests/test-expected.txt', 'LayoutTests/no-test-expected.txt']
        fs = MockFileSystem()
        fs.write_text_file('/test.checkout/LayoutTests/test.html', 'This is a test')
        fs, touched_tests = self.touched_files(paths, fs)
        self.assertEqual(touched_tests, ['test.html'])

    def test_platform_expected_touched_test(self):
        paths = ['LayoutTests/platform/mock/test-expected.txt', 'LayoutTests/platform/mock/no-test-expected.txt']
        fs = MockFileSystem()
        fs.write_text_file('/test.checkout/LayoutTests/test.html', 'This is a test')
        fs, touched_tests = self.touched_files(paths, fs)
        self.assertEqual(touched_tests, ['test.html'])

    def test_platform_duplicate_touched_test(self):
        paths = ['LayoutTests/test1.html', 'LayoutTests/test1.html', 'LayoutTests/platform/mock1/test2-expected.txt', 'LayoutTests/platform/mock2/test2-expected.txt']
        fs = MockFileSystem()
        fs.write_text_file('/test.checkout/LayoutTests/test2.html', 'This is a test')
        fs, touched_tests = self.touched_files(paths, fs)
        self.assertEqual(sorted(touched_tests), sorted(['test1.html', 'test2.html']))

    def test_touched_but_skipped_test(self):
        host = MockHost()
        port = TestPort(host)

        expectations_dict = OrderedDict()
        expectations_dict['expectations'] = 'test1.html [ Skip ]\ntest3.html [ Skip ]\n'
        port.expectations_dict = lambda **kwargs: expectations_dict
        port.test_exists = lambda test: True

        paths = ['LayoutTests/test0.html', 'LayoutTests/test1.html', 'LayoutTests/test2-expected.txt', 'LayoutTests/test3-expected.txt']
        host.filesystem.write_text_file('/test.checkout/LayoutTests/test2.html', 'This is a test to be runned')
        host.filesystem.write_text_file('/test.checkout/LayoutTests/test3.html', 'This is a test to be skipped')

        touched_tests = MockLayoutTestFinder(port, optparse.Values({'skipped': 'always', 'skip_failing_tests': False, 'http': True})).find_touched_tests(paths)
        self.assertEqual(sorted(touched_tests), sorted(['test0.html', 'test2.html']))
