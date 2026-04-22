# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/TCD/IoT_older/esp-idf-v4.4.7/components/bootloader/subproject"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix/tmp"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix/src"
  "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/liam0/esp/esp-dev-kits/examples/esp32-s2-kaluga-1/examples/lcd/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
