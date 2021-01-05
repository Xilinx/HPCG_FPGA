#!/usr/bin/python
import os
import sys

  
#
# Copyright (C) 2021 Xilinx, Inc
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#


def main ():
    dev = sys.argv[1]
    if "PLATFORM_REPO_PATHS" in os.environ:
        plist = os.environ['PLATFORM_REPO_PATHS'].split(":")
        for shell in plist:
            if os.path.isdir(shell + "/" + dev):
                return shell

print (main())
