########################################################################
# Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer         #
#                                                                      #
# This file is part of DJMatch and licensed under GPLv3.               #
########################################################################

QT =

CONFIG += c++17 c++1z

TARGET = DJMatch
CONFIG -= app_bundle

TEMPLATE = app

DJMINFOHDRTMPL = $$PWD/djmatch_info.TEMPLATE.h
DJMINFOHDR = $$PWD/djmatch_info.h
djminfotarget.target =  $$DJMINFOHDR
djminfotarget.commands = '$$PWD/../updateInfoHeader $$DJMINFOHDRTMPL $$DJMINFOHDR'
djminfotarget.depends = FORCE $$DJMINFOHDRTMPL
PRE_TARGETDEPS += $$DJMINFOHDR
QMAKE_EXTRA_TARGETS += djminfotarget

QMAKE_CXXFLAGS_APP =
QMAKE_CXXFLAGS_STATIC_LIB = # remove -fPIC

QMAKE_CXXFLAGS_DEBUG += -std=c++17 -O0
QMAKE_LFLAGS_DEBUG +=

QMAKE_CXXFLAGS_RELEASE -= -O3 -O2 -O1
QMAKE_CXXFLAGS_RELEASE += -std=c++17 -DNDEBUG -flto
QMAKE_LFLAGS_RELEASE += -flto -O3

custom-ar {
  QMAKE_AR += rcs
} else {
  QMAKE_AR -= cqs
  QMAKE_AR -= cq
  QMAKE_AR += rcs
}

general {
  QMAKE_CXXFLAGS_RELEASE += -O2 -march=x86-64
} else {
  QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -mtune=native
}

debugsymbols {
	QMAKE_CXXFLAGS_RELEASE += -fno-omit-frame-pointer -g
}

SOURCES += extern/argtable3-3.0.3/argtable3.c \
      main.cpp \
      matching/node_centered.cpp \
      matching/greedy_iterative.cpp \
      matching/greedy_b_matching.cpp \
      matching/gpa/gpa.cpp \
      matching/coloring/misra_gries.cpp

HEADERS += extern/argtable3-3.0.3/argtable3.h \
      tools/chronotimer.h \
      djmatch_info.h \
      matching/matching_defs.h \
      matching/matching_config.h \
      matching/matching_algorithm.h \
      matching/node_centered.h \
      matching/greedy_iterative.h \
      matching/greedy_b_matching.h \
      matching/gpa/path.h \
      matching/gpa/path_set.h \
      matching/gpa/gpa.h \
      matching/coloring/misra_gries.h \
      matching/coloring/k_edge_coloring.hpp

CONFIG(release, debug|release) {
  unix:!macx: LIBS += -L$$PWD/../Algora/AlgoraDyn/build/Release/ -lAlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../Algora/AlgoraCore/build/Release/ -lAlgoraCore
}
CONFIG(debug, debug|release) {
  unix:!macx: LIBS += -L$$PWD/../Algora/AlgoraDyn/build/Debug/ -lAlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../Algora/AlgoraCore/build/Debug/ -lAlgoraCore
}

INCLUDEPATH += $$PWD/../Algora/AlgoraCore/src
DEPENDPATH += $$PWD/../Algora/AlgoraCore/src
unix:!macx: PRE_TARGETDEPS += $$PWD/../Algora/AlgoraCore/build/Debug/libAlgoraCore.a

INCLUDEPATH += $$PWD/../Algora/AlgoraDyn/src
DEPENDPATH += $$PWD/../Algora/AlgoraDyn/src
unix:!macx: PRE_TARGETDEPS += $$PWD/../Algora/AlgoraDyn/build/Debug/libAlgoraDyn.a
