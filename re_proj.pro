QT += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    grafwidget.cpp \
    main.cpp \
    dialog.cpp \
    dht22.cpp

HEADERS += \
    dialog.h \
    grafwidget.h \
    dht22.h

FORMS += \
    dialog.ui

else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    slike.qrc

LIBS += -lpigpiod_if2 -lrt
