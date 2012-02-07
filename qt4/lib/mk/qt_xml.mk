include $(REP_DIR)/lib/import/import-qt_xml.mk

SHARED_LIB = yes

# extracted from src/xml/Makefile
QT_DEFINES += -DQT_BUILD_XML_LIB -DQT_NO_USING_NAMESPACE -DQT_NO_CAST_TO_ASCII -DQT_ASCII_CAST_WARNINGS -DQT_MOC_COMPAT -DQT_NO_DEBUG -DQT_CORE_LIB

# extracted from src/xml/Makefile
QT_SOURCES = \
         qdom.cpp \
         qxml.cpp

# some source files need to be generated by moc from other source/header files before
# they get #included again by the original source file in the compiling stage

# source files generated from existing header files ("moc_%.cpp: %.h" rule in spec-qt4.mk)
# extracted from "compiler_moc_header_make_all" target
COMPILER_MOC_HEADER_MAKE_ALL_FILES = \

# source files generated from existing source files ("%.moc: %.cpp" rule in spec-qt4.mk)
# extracted from "compiler_moc_source_make_all" rule
COMPILER_MOC_SOURCE_MAKE_ALL_FILES = \

INC_DIR += $(REP_DIR)/src/lib/qt4/mkspecs/qws/genode-x86-g++ \
           $(REP_DIR)/include/qt4 \
           $(REP_DIR)/contrib/$(QT4)/include \
           $(REP_DIR)/include/qt4/QtCore \
           $(REP_DIR)/contrib/$(QT4)/include/QtCore \
           $(REP_DIR)/include/qt4/QtCore/private \
           $(REP_DIR)/contrib/$(QT4)/include/QtCore/private \
           $(REP_DIR)/include/qt4/QtXml \
           $(REP_DIR)/contrib/$(QT4)/include/QtXml \
           $(REP_DIR)/include/qt4/QtXml/private \
           $(REP_DIR)/contrib/$(QT4)/include/QtXml/private \
           $(REP_DIR)/src/lib/qt4/src/corelib/global

LIBS += qt_core libc

vpath % $(REP_DIR)/include/qt4/QtXml
vpath % $(REP_DIR)/include/qt4/QtXml/private

vpath % $(REP_DIR)/src/lib/qt4/src/xml/dom
vpath % $(REP_DIR)/src/lib/qt4/src/xml/sax

vpath % $(REP_DIR)/contrib/$(QT4)/src/xml/dom
vpath % $(REP_DIR)/contrib/$(QT4)/src/xml/sax

include $(REP_DIR)/lib/mk/qt.mk
