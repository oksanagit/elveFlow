< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
elveFlowApp_registerRecordDeviceDriver pdbbase

## OKS Not sure what to lad dbLoadTemplate("1608G.substitutions_V1")

## Configure port driver
# USBelveFlowConfig(portName)        # The name to give to this asyn port driver

USBelveFlowConfig("1608G_1", 1)

#asynSetTraceMask 1608G_1 -1 255

iocInit
