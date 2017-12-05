TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    protocol/ChatRoom.pb.cc \
    protocol/CommunicationData.pb.cc \
    make_pack.cpp \
    global.cpp \
    parse_pack.cpp \
    json11/json11.cpp

LIBS += -lboost_thread -lboost_system -lboost_program_options -lpthread -lssl -lcrypto -lz

DISTFILES += \
    LICENSE

HEADERS += \
    protocol/ChatRoom.pb.h \
    protocol/CommunicationData.pb.h \
    make_pack.hpp \
    global.hpp \
    parse_pack.hpp \
    json11/json11.hpp
