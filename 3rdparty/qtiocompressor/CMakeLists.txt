cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)


set(IOCOMPRESSOR-SOURCES
    qtiocompressor.cpp
)

set(IOCOMPRESSOR-MOC-HEADERS
    qtiocompressor.h
)


ADD_LIBRARY(qtiocompressor STATIC
    ${IOCOMPRESSOR-SOURCES}
    ${IOCOMPRESSOR-SOURCES-MOC}
)
