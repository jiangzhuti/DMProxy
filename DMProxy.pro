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
    json11/json11.cpp \
    utils/md5.cpp \
    utils/rc4.cpp \
    utils/gzip.cpp

LIBS += -lboost_thread -lboost_system -lboost_program_options -lpthread -lssl -lcrypto -lz

DISTFILES += \
    LICENSE

HEADERS += \
    protocol/ChatRoom.pb.h \
    protocol/CommunicationData.pb.h \
    make_pack.hpp \
    global.hpp \
    parse_pack.hpp \
    json11/json11.hpp \
    utils/md5.hpp \
    utils/rc4.hpp \
    utils/gzip.hpp
