#!/bin/bash

engine=${engine:-"null"}
proto=${proto:-"null"}
games=${games:-"100"}

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
-each tc=4/6 timemargin=0 -games $games -repeat \
-recover -concurrency $(nproc)
