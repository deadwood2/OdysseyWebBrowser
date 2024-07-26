#!/usr/bin/env python

import json
import logging
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import types
import os
import urlparse

from benchmark_builder import BenchmarkBuilder
from benchmark_results import BenchmarkResults
from browser_driver.browser_driver_factory import BrowserDriverFactory
from http_server_driver.http_server_driver_factory import HTTPServerDriverFactory
from utils import timeout


_log = logging.getLogger(__name__)


class BenchmarkRunner(object):

    def __init__(self, plan_file, local_copy, count_override, build_dir, output_file, platform, browser, device_id=None):
        try:
            plan_file = self._find_plan_file(plan_file)
            with open(plan_file, 'r') as fp:
                self._plan_name = os.path.split(os.path.splitext(plan_file)[0])[1]
                self._plan = json.load(fp)
                if local_copy:
                    self._plan['local_copy'] = local_copy
                if count_override:
                    self._plan['count'] = count_override
                self._browser_driver = BrowserDriverFactory.create(platform, browser)
                self._http_server_driver = HTTPServerDriverFactory.create(platform)
                self._http_server_driver.set_device_id(device_id)
                self._build_dir = os.path.abspath(build_dir) if build_dir else None
                self._output_file = output_file
                self._device_id = device_id
        except IOError as error:
            _log.error('Can not open plan file: {plan_file} - Error {error}'.format(plan_file=plan_file, error=error))
            raise error
        except ValueError as error:
            _log.error('Plan file: {plan_file} may not follow JSON format - Error {error}'.format(plan_file=plan_file, error=error))
            raise error

    def _find_plan_file(self, plan_file):
        if not os.path.exists(plan_file):
            absPath = os.path.join(os.path.dirname(__file__), 'data/plans', plan_file)
            if os.path.exists(absPath):
                return absPath
            if not absPath.endswith('.plan'):
                absPath += '.plan'
            if os.path.exists(absPath):
                return absPath
        return plan_file

    def _run_benchmark(self, count, web_root):
        results = []
        for iteration in xrange(1, count + 1):
            _log.info('Start the iteration {current_iteration} of current benchmark'.format(current_iteration=iteration))
            try:
                result = None
                self._http_server_driver.serve(web_root)
                self._browser_driver.prepare_env(self._device_id)
                url = urlparse.urljoin(self._http_server_driver.base_url(), self._plan_name + '/' + self._plan['entry_point'])
                self._browser_driver.launch_url(url, self._build_dir)
                with timeout(self._plan['timeout']):
                    result = self._http_server_driver.fetch_result()
                    assert(not self._http_server_driver.get_return_code())
                    assert(result)
                    results.append(json.loads(result))
            finally:
                self._browser_driver.restore_env()
                self._browser_driver.close_browsers()
                self._http_server_driver.kill_server()
            _log.info('End of {current_iteration} iteration of current benchmark'.format(current_iteration=iteration))
        results = self._wrap(results)
        self._dump(results, self._output_file if self._output_file else self._plan['output_file'])
        self._show_results(results)

    def execute(self):
        with BenchmarkBuilder(self._plan_name, self._plan) as web_root:
            self._run_benchmark(int(self._plan['count']), web_root)

    @classmethod
    def _dump(cls, results, output_file):
        _log.info('Dumping the results to file')
        try:
            with open(output_file, 'w') as fp:
                json.dump(results, fp)
        except IOError as error:
            _log.error('Cannot open output file: {output_file} - Error: {error}'.format(output_file=output_file, error=error))
            _log.error('Results are:\n {result}'.format(json.dumps(results)))

    @classmethod
    def _wrap(cls, dicts):
        _log.debug('Merging following results:\n{results}'.format(results=json.dumps(dicts)))
        if not dicts:
            return None
        ret = {}
        for dic in dicts:
            ret = cls._merge(ret, dic)
        _log.debug('Results after merging:\n{result}'.format(result=json.dumps(ret)))
        return ret

    @classmethod
    def _merge(cls, a, b):
        assert(isinstance(a, type(b)))
        arg_type = type(a)
        # special handle for list type, and should be handle before equal check
        if arg_type == types.ListType and len(a) and (type(a[0]) == types.StringType or type(a[0]) == types.UnicodeType):
            return a
        if arg_type == types.DictType:
            result = {}
            for key, value in a.items():
                if key in b:
                    result[key] = cls._merge(value, b[key])
                else:
                    result[key] = value
            for key, value in b.items():
                if key not in result:
                    result[key] = value
            return result
        # for other types
        return a + b

    @classmethod
    def _show_results(cls, results):
        results = BenchmarkResults(results)
        print results.format()
