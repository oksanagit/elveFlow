< envPaths

## Register all support components
dbLoadDatabase "../../dbd/elveFlowApp.dbd"
elveFlowApp_registerRecordDeviceDriver pdbbase

dbLoadTemplate("elveFlow.substitutions")

## Configure port driver
# USBelveFlowConfig(portName)        # The name to give to this asyn port driver

USBelveFlowConfig("elveFlowOB1", 1)

#asynSetTraceMask elveFlowOB1 -1 255

iocInit
