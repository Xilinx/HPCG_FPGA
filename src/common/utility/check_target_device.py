import json, sys

  
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


descriptionfile = sys.argv[1]
target = sys.argv[2]
device = sys.argv[3]

with open(descriptionfile) as json_file:
    data = json.load(json_file)

targetNotSupported = 'targets' in data and target not in data['targets']
if targetNotSupported:
    print("%s target not supported for this example" % target)

deviceNotSupported = 'nboard' in data and any(nboard in device for nboard in data['nboard'])
if deviceNotSupported:
    print("%s device not supported for this example" % device)

sys.exit(not(targetNotSupported or deviceNotSupported))
