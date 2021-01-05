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
# utility that creates a file that lists down the supported and unsupported devices
# for each example
#

import glob
import json
import re
import sys
import os

import os.path

string = "" 
for dirpath, dirnames, filenames in os.walk("../../."):   
	for filename in [f for f in filenames if (f.endswith("description.json") and f not in "../../common/.")]:
	
		f = open(os.path.join(dirpath, filename), "r+")
		listing = []
		flag = 0 
		name_flag = 0

		for txt in f:

			x = re.search(".*device\".*", txt)

			if (x):
				if(name_flag is 0):
                                        name_flag = 1
                                        string = string + "\n" + dirpath + "\n"

				if(',' not in txt):	
					flag = 1
				
				string = string + txt
				continue	

			if (flag):
				string = string + txt
				
				if(']' in txt):
					flag = 0
		f.close()

g = open ("Data.txt", "w")
g.write(string)
g.close()
	
