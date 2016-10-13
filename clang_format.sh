#!/bin/sh

find sources includes tests examples \( -name '*.cpp' -o -name '*.hpp' -o -name '*.ipp' -o -name '*.c' -o -name '*.h' \) -exec clang-format -i {} ';'
