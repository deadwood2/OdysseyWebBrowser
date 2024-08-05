#!/usr/bin/env python

import json
import logging
import urlparse

from benchmark_runner import BenchmarkRunner
from http_server_driver.http_server_driver_factory import HTTPServerDriverFactory
from utils import timeout


_log = logging.getLogger(__name__)


class WebServerBenchmarkRunner(BenchmarkRunner):
    name = 'webserver'

    def __init__(self, plan_file, local_copy, count_override, build_dir, output_file, platform, browser, scale_unit=True, device_id=None):
        self._http_server_driver = HTTPServerDriverFactory.create(platform)
        self._http_server_driver.set_device_id(device_id)
        super(WebServerBenchmarkRunner, self).__init__(plan_file, local_copy, count_override, build_dir, output_file, platform, browser, scale_unit, device_id)

    def _get_result(self, test_url):
        result = self._browser_driver.add_additional_results(test_url, self._http_server_driver.fetch_result())
        assert(not self._http_server_driver.get_return_code())
        return result

    def _run_one_test(self, web_root, test_file):
        result = None
        try:
            self._http_server_driver.serve(web_root)
            url = urlparse.urljoin(self._http_server_driver.base_url(), self._plan_name + '/' + test_file)
            self._browser_driver.launch_url(url, self._plan['options'], self._build_dir)
            with timeout(self._plan['timeout']):
                result = self._get_result(url)
        finally:
            self._browser_driver.close_browsers()
            self._http_server_driver.kill_server()

        return json.loads(result)
