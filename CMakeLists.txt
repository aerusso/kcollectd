cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0011 OLD)

project(kcollectd)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)

subdirs(kcollectd po)
