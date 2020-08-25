PLUGIN = qthotkey${PLUGIN_SUFFIX}

include ../../extra.mk
SRCS = plugin.cc gui.cc thirdparty/qhotkey.cc thirdparty/qhotkey.moc.cc  thirdparty/qhotkey_p.moc.cc
ifeq ($(HAVE_LINUX),yes)
SRCS += qhotkey_x11.cc
else
  ifeq ($(HAVE_MSWINDOWS),yes)
  	SRCS += thirdparty/qhotkey_win.cc
  else # assume "darwin"
    SRCS += thirdparty/qhotkey_mac.cc
  endif
endif

include ../../buildsys.mk
include ../../extra.mk


# $(info PluginName added is ${PLUGIN_SUFFIX} or $(PLUGIN_SUFFIX) )
# $(info The makefile for keys!! L $(HAVE_LINUX) win $(HAVE_MSWINDOWS) mac $(HAVE_DARWIN))



plugindir := ${plugindir}/${GENERAL_PLUGIN_DIR}

LD = ${CXX}
CFLAGS += ${PLUGIN_CFLAGS}
CPPFLAGS += ${PLUGIN_CPPFLAGS} ${QT_CFLAGS}
LIBS += ${QT_LIBS}

ifeq ($(HAVE_MSWINDOWS),no)
CPPFLAGS += ${QTX11EXTRAS_CFLAGS}
LIBS += ${QTX11EXTRAS_LIBS} -lX11
else
endif


CPPFLAGS += -I../.. -I..
LIBS += -laudqt

%.moc.cc: %.h
	moc $< -o $@

clean: myclean

myclean:
	rm thirdparty/qhotkey.moc.cc  thirdparty/qhotkey_p.moc.cc

#%.moc.o: %.moc.cc
#	@echo "Building Moc: $@ from $<"
#	$(CXXCOMPILE) $(QT_CPPFLAGS) $(QT_CXXFLAGS) -c $< -o $@
#
#%.moc.cc: %.h
#	@echo " -- Building Moc1111: $@ from $<"
#	$(MOC) $< -o $@
#
#%.moc: %.h
#	moc $< -o $@

#qhotkey_win.cc: qhotkey_win.h
#        moc $(DEFINES) $(INCPATH) $< -o $@

#qhotkey.plugin.o: foo.moc
#
#foo.moc: foo.cpp
#        moc $(DEFINES) $(INCPATH) -i $< -o $@