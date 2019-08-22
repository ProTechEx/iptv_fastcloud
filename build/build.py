#!/usr/bin/env python3
import sys
from pyfastogt import build_utils, system_info


class BuildRequest(build_utils.BuildRequest):
    def __init__(self, platform, arch_name, dir_path, prefix_path):
        build_utils.BuildRequest.__init__(self, platform, arch_name, dir_path, prefix_path)

    def build(self, license_key, license_algo, build_type):
        cmake_flags = ['-DLICENSE_KEY=%s' % license_key, '-DLICENSE_ALGO=%s' % license_algo]
        self._build_via_cmake_double(cmake_flags, build_type)


def print_usage():
    print("Usage:\n"
          "[required] argv[1] build type(release/debug)\n"
          "[required] argv[2] license key\n"
          "[optional] argv[3] license algo\n"
          "[optional] argv[4] prefix\n")


if __name__ == "__main__":
    argc = len(sys.argv)

    if argc > 1:
        build_type = sys.argv[1]
    else:
        print_usage()
        sys.exit(1)

    if argc > 2:
        license_key = sys.argv[2]
    else:
        print_usage()
        sys.exit(1)

    license_algo = 0
    if argc > 3:
        license_algo = sys.argv[3]

    prefix = '/usr/local'
    if argc > 4:
        prefix = sys.argv[4]

    host_os = system_info.get_os()
    arch_host_os = system_info.get_arch_name()

    request = BuildRequest(host_os, arch_host_os, 'build_' + host_os, prefix)
    request.build(license_key, license_algo, build_type)
