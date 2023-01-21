#!/bin/bash

# Copyright 2023 En-En-Code
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

engine=${engine:-"null"}
proto=${proto:-"null"}
games=${games:-"1000"}

while [ $# -gt 0 ]; do
  if [[ $1 = *"--"* ]]; then
    declare $"${1/--/}"="$2"
    shift
  fi
  
  shift
done

if [ ${engine} = "null" ]; then
  echo "Necessary parameter 'engine' not provided."
  echo "Usage: ./Tourney-No-Inc-Test.sh --engine <engine> --proto <xboard|uci>"
  exit 1
fi
if [[ ${proto} != "uci" && ${proto} != "xboard" ]]; then
  echo "Necessary parameter 'proto' not 'uci' or 'xboard'."
  echo "Usage: ./Tourney-No-Inc-Test.sh --engine <engine> --proto <xboard|uci>"
  exit 1
fi

cutechess-cli \
-engine cmd=$engine proto=$proto \
-engine cmd=$engine proto=$proto \
-each tc=4/0.1 timemargin=0 -games $games -repeat \
-recover -concurrency $(nproc)
