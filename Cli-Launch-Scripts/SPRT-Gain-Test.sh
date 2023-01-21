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

engine1=${engine1:-"null"}
engine2=${engine2:-"null"}
proto=${proto:-"null"}
openingfile=${openingfile:""}
games=${games:-"100000"}

while [ $# -gt 0 ]; do
  if [[ $1 = *"--"* ]]; then
    declare $"${1/--/}"="$2"
    shift
  fi
  
  shift
done

if [ ${engine1} = "null" ]; then
  echo "Necessary parameter 'engine1' not provided."
  echo "Usage: ./Common-SPRT-Test.sh --engine1 <engine> --engine2 <engine> --proto <xboard|uci>"
  exit 1
fi
if [ ${engine2} = "null" ]; then
  echo "Necessary parameter 'engine2' not provided."
  echo "Usage: ./Common-SPRT-Test.sh --engine1 <engine> --engine2 <engine> --proto <xboard|uci>"
  exit 1
fi
if [[ ${proto} != "uci" && ${proto} != "xboard" ]]; then
  echo "Necessary parameter 'proto' not 'uci' or 'xboard'."
  echo "Usage: ./Common-SPRT-Test.sh --engine1 <engine> --engine2 <engine> --proto <xboard|uci>"
  exit 1
fi

cutechess-cli \
-engine cmd=$engine1 proto=$proto \
-engine cmd=$engine2 proto=$proto \
-each tc=1+0.01 timemargin=0 -games $games -repeat \
-recover -concurrency $(nproc) \
-draw movenumber=0 movecount=10 score=10 \
-resign movecount=10 score=750 \
-sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 \
-openings file=$openingfile order=random -bookmode disk \
