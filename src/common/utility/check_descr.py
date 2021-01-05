#!/usr/bin/env python

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


#
# utility that lists down
# all the examples with respective unnecessary keywords mentioned
#

import glob
import json
import re
import sys
import os

import os.path

for dirpath, dirnames, filenames in os.walk("../.././"):   
        for filename in [f for f in filenames if (f.endswith("description.json") and f not in "../../common/.")]:

                f = open(os.path.join(dirpath, filename), "r+")
                flag = 0
                t = 0
                string_check = ""

                for txt in f:
                        if ("keywords" in txt and flag == 0):
                                flag = 1
                                continue

                        if (flag):
                                if('}' in txt or ']' in txt):
                                        break

                                else:
                                        c_list = txt.split("\"")
                                        check_flag = 0
                                        for c_dirpath, c_dirnames, c_filenames in os.walk(os.path.join(dirpath)):
                                                for check_filename in [c_f for c_f in c_filenames if (not (c_f.endswith(".md") or c_f.endswith("description.json")))]:
                                                        c_f = open(os.path.join(c_dirpath, check_filename), "rb+")

                                                        for check_txt in c_f:
                                                                if (c_list[1].encode('utf-8') in check_txt):
                                                                        check_flag = 1
                                                                        break
						
                                                        c_f.close()

                                        if (check_flag is 0):
                                                string_check = string_check + txt
                                                t = 1

                if (t):
                        print(os.path.join(dirpath))
                        print(string_check)
		
                f.close()
