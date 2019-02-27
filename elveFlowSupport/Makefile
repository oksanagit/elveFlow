TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Note, the following files were manually copied from the Measurement Computing/DAQ/C directory.
INC += Elveflow64.h
INC += extcode.h
INC += platdefines.h
INC += fundtypes.h
INC += lv_prolog.h

USR_CFLAGS_WIN32 += /MANIFESTUAC:level=asInvoker
USR_CFLAGS_WIN32 += /MANIFESTUAC:uiAccess=false


INC += lv_epilog.h

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS += ../os/win32-x86/Elveflow32.lib

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS += ../os/windows-x64/Elveflow64.lib

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

