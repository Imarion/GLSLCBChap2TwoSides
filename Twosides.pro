QT += gui core

CONFIG += c++11

TARGET = twosides
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    twosides.cpp \
    vertex.cpp \
    vertexcol.cpp \
    vertextex.cpp \
    teapot.cpp

HEADERS += \
    twosides.h \
    vertex.h \
    vertexcol.h \
    vertextex.h \
    teapotdata.h \
    teapot.h

OTHER_FILES += \
    vshader_2sides.txt \
    fshader_2sides.txt \
    fshader_ads.txt \
    vshader_ads.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    vshader_2sides.txt \
    fshader_2sides.txt \
    fshader_ads.txt \
    vshader_ads.txt
