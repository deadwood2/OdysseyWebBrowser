# Copyright (C) 2017 Igalia S.L.
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

import os
from webkitpy.webdriver_tests.webdriver_driver import WebDriver, register_driver


class WebDriverWPE(WebDriver):

    def __init__(self, port):
        super(WebDriverWPE, self).__init__(port)

    def binary_path(self):
        return self._port._build_path('bin', 'WPEWebDriver')

    def browser_name(self):
        return 'dyz'

    def browser_args(self):
        return ['--automation']

    def capabilities(self):
        return {'wpe:browserOptions': {
            'binary': self.browser_name(),
            'args': self.browser_args()}}

    def browser_env(self):
        env = {}
        env['WEBKIT_EXEC_PATH'] = self._port._build_path('bin')
        try:
            ld_library_path = os.environ['LD_LIBRARY_PATH']
        except KeyError:
            ld_library_path = None
        env['LD_LIBRARY_PATH'] = self._port._build_path('lib')
        if ld_library_path:
            env['LD_LIBRARY_PATH'] += ':' + ld_library_path
        return env


register_driver('wpe', WebDriverWPE)
