TOP = ..
include $(TOP)/configure/CONFIG
##cbw_linuxSrc_DEPEND_DIRS += Linux_Drivers
##DIRS := $(DIRS) src
##src_DEPEND_DIRS += cbw_linuxSrc
DIRS := $(DIRS) demoSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

