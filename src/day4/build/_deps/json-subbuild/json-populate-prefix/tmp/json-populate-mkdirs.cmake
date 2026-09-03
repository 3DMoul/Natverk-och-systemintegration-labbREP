# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-src")
  file(MAKE_DIRECTORY "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-build"
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix"
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/tmp"
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/src"
  "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/svant/OneDrive/Desktop/dagens labb rep/Natverk-och-systemintegration-labbREP/src/day4/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
