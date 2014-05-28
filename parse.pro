#-------------------------------------------------
#
# Project created by QtCreator 2014-05-27T21:55:11
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = parse
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    scpiparser.cpp \
    utils.c

HEADERS += \
    scpiparser.h \
    types.h \
    config.h \
    constants.h \
    error.h \
    ieee488.h \
    utils_private.h \
    fifo.h
