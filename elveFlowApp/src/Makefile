TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

DBD += elveFlowApp.dbd
DBD += elveFlowSupport.dbd

##LIBRARY_IOC_WIN32 += Elveflow
# Compile and add the code to the support library
# Link locally-provided code into the support library,
# rather than directly into the IOC application.

## LIB_SRCS += drvElveFlowOB1.cpp

##Elveflow_LIBS += asyn
###Elveflow_LIBS += Elveflow64
##Elveflow_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC_WIN32 += elveFlowAppV1
### elveFlowAppV1_SRCS += drvElveFlowOB1

PROD_SRCS += elveFlowApp_registerRecordDeviceDriver.cpp
PROD_SRCS += elveFlowAppMain.cpp
PROD_LIBS += asyn
PROD_LIBS += Elveflow
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
PROD_LIBS += cbw_32
else ifeq (cygwin-x86, $(findstring cygwin-x86, $(T_A)))
PROD_LIBS += cbw_32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
PROD_LIBS += Elveflow64
endif
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

