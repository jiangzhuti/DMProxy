TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.cpp

LIBS += -lboost_thread -lboost_system -lboost_program_options -lpthread

DISTFILES += \
    LICENSE
