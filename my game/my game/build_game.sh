#!/usr/bin/env bash
cd "/c/Users/JOHN EFRAIM PASTRANA/Downloads/my game/my game"
set -e
/ucrt64/bin/g++ -std=c++14 -Wall -O1 src/*.cpp -I/c/msys64/ucrt64/include -L/c/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -o game_updated_build.exe 2>&1 | tee build_all_output.txt
echo "EXIT:$?"
ls -l game_updated_build.exe || true
