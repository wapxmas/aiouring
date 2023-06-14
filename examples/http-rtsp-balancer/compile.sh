#!/usr/bin/env bash
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Release -Bbuildbin -H.
cmake --build buildbin -- -j 8
