include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += ./Neptune/Source/Core ./Platinum/Source/Core ./Platinum/Source/Platinum ./Platinum/Source/Devices/MediaServer ./Neptune/Source/System/Posix
INCLUDEPATH += ../../../

SOURCES = Platinum/Source/Core/PltAction.cpp \
      Platinum/Source/Core/PltArgument.cpp \
      Platinum/Source/Core/PltConstants.cpp \
      Platinum/Source/Core/PltCtrlPoint.cpp \
      Platinum/Source/Core/PltCtrlPointTask.cpp \
      Platinum/Source/Core/PltDatagramStream.cpp \
      Platinum/Source/Core/PltDeviceData.cpp \
      Platinum/Source/Core/PltDeviceHost.cpp \
      Platinum/Source/Core/PltEvent.cpp \
      Platinum/Source/Core/PltHttp.cpp \
      Platinum/Source/Core/PltHttpClientTask.cpp \
      Platinum/Source/Core/PltHttpServer.cpp \
      Platinum/Source/Core/PltHttpServerTask.cpp \
      Platinum/Source/Core/PltService.cpp \
      Platinum/Source/Core/PltSsdp.cpp \
      Platinum/Source/Core/PltStateVariable.cpp \
      Platinum/Source/Core/PltStreamPump.cpp \
      Platinum/Source/Core/PltTaskManager.cpp \
      Platinum/Source/Core/PltThreadTask.cpp \
      Platinum/Source/Core/PltUPnP.cpp \
      Platinum/Source/Core/PltTime.cpp \
      Platinum/Source/Devices/MediaServer/PltDidl.cpp \
      Platinum/Source/Devices/MediaServer/PltFileMediaServer.cpp \
      Platinum/Source/Devices/MediaServer/PltMediaBrowser.cpp \
      Platinum/Source/Devices/MediaServer/PltMediaCache.cpp \
      Platinum/Source/Devices/MediaServer/PltMediaItem.cpp \
      Platinum/Source/Devices/MediaServer/PltMediaServer.cpp \
      Platinum/Source/Devices/MediaServer/ContentDirectorywSearchSCPD.cpp \
      Platinum/Source/Devices/MediaServer/ConnectionManagerSCPD.cpp \
      Platinum/Source/Devices/MediaServer/PltSyncMediaBrowser.cpp \
      Neptune/Source/Core/Neptune.cpp \
      Neptune/Source/Core/NptBase64.cpp \
      Neptune/Source/Core/NptBufferedStreams.cpp \
      Neptune/Source/Core/NptCommon.cpp \
      Neptune/Source/Core/NptDataBuffer.cpp \
      Neptune/Source/Core/NptDebug.cpp \
      Neptune/Source/Core/NptFile.cpp \
      Neptune/Source/Core/NptHttp.cpp \
      Neptune/Source/Core/NptList.cpp \
      Neptune/Source/Core/NptMessaging.cpp \
      Neptune/Source/Core/NptNetwork.cpp \
      Neptune/Source/Core/NptQueue.cpp \
      Neptune/Source/Core/NptRingBuffer.cpp \
      Neptune/Source/Core/NptSimpleMessageQueue.cpp \
      Neptune/Source/Core/NptSockets.cpp \
      Neptune/Source/Core/NptStreams.cpp \
      Neptune/Source/Core/NptStrings.cpp \
      Neptune/Source/Core/NptSystem.cpp \
      Neptune/Source/Core/NptThreads.cpp \
      Neptune/Source/Core/NptTime.cpp \
      Neptune/Source/Core/NptUri.cpp \
      Neptune/Source/Core/NptUtils.cpp \
      Neptune/Source/Core/NptXml.cpp \
      Neptune/Source/Core/NptLogging.cpp \
      Neptune/Source/Core/NptResults.cpp \
      Neptune/Source/System/Bsd/NptBsdSockets.cpp \
      Neptune/Source/System/Bsd/NptBsdNetwork.cpp \
      Neptune/Source/System/Linux/NptLinuxNetwork.cpp \
      Neptune/Source/System/Posix/NptPosixSystem.cpp \
      Neptune/Source/System/Posix/NptSelectableMessageQueue.cpp \
      Neptune/Source/System/Posix/NptPosixQueue.cpp \
      Neptune/Source/System/Posix/NptPosixThreads.cpp \
      Neptune/Source/System/Posix/NptPosixFile.cpp \
      Neptune/Source/System/StdC/NptStdCTime.cpp \
      Neptune/Source/System/StdC/NptStdcDebug.cpp \
      Neptune/Source/System/StdC/NptStdcEnvironment.cpp \
      Platinum/Source/Devices/MediaRenderer/PltMediaRenderer.cpp \
      Platinum/Source/Devices/MediaRenderer/PltMediaController.cpp \
      Platinum/Source/Devices/MediaRenderer/AVTransportSCPD.cpp \
      Platinum/Source/Devices/MediaRenderer/RdrConnectionManagerSCPD.cpp \
      Platinum/Source/Devices/MediaRenderer/RenderingControlSCPD.cpp \
      Platinum/Source/Devices/MediaConnect/X_MS_MediaReceiverRegistrarSCPD.cpp \
      Platinum/Source/Devices/MediaConnect/PltMediaConnect.cpp \


HEADERS = Platinum/Source/Core/PltAction.h \
      Platinum/Source/Core/PltArgument.h \
      Platinum/Source/Core/PltConstants.h \
      Platinum/Source/Core/PltCtrlPoint.h \
      Platinum/Source/Core/PltCtrlPointTask.h \
      Platinum/Source/Core/PltDatagramStream.h \
      Platinum/Source/Core/PltDeviceData.h \
      Platinum/Source/Core/PltDownloader.h \
      Platinum/Source/Core/PltEvent.h \
      Platinum/Source/Core/PltHttp.h \
      Platinum/Source/Core/PltHttpClientTask.h \
      Platinum/Source/Core/PltHttpServer.h \
      Platinum/Source/Core/PltHttpServerTask.h \
      Platinum/Source/Core/PltService.h \
      Platinum/Source/Core/PltSsdp.h \
      Platinum/Source/Core/PltStateVariable.h \
      Platinum/Source/Core/PltStreamPump.h \
      Platinum/Source/Core/PltTaskManager.h \
      Platinum/Source/Core/PltThreadTask.h \
      Platinum/Source/Core/PltUPnP.h \
      Platinum/Source/Core/PltTime.h \
      Platinum/Source/Devices/MediaServer/PltDidl.h \
      Platinum/Source/Devices/MediaServer/PltFileMediaServer.h \
      Platinum/Source/Devices/MediaServer/PltMediaBrowser.h \
      Platinum/Source/Devices/MediaServer/PltMediaCache.h \
      Platinum/Source/Devices/MediaServer/PltMediaItem.h \
      Platinum/Source/Devices/MediaServer/PltMediaServer.h \
      Platinum/Source/Devices/MediaServer/ContentDirectorywSearchSCPD.h \
      Platinum/Source/Devices/MediaServer/ConnectionManagerSCPD.h \
      Platinum/Source/Devices/MediaServer/PltSyncMediaBrowser.h \
      Neptune/Source/Core/Neptune.h \
      Neptune/Source/Core/NptBase64.h \
      Neptune/Source/Core/NptBufferedStreams.h \
      Neptune/Source/Core/NptCommon.h \
      Neptune/Source/Core/NptDataBuffer.h \
      Neptune/Source/Core/NptDebug.h \
      Neptune/Source/Core/NptFile.h \
      Neptune/Source/Core/NptHttp.h \
      Neptune/Source/Core/NptList.h \
      Neptune/Source/Core/NptMessaging.h \
      Neptune/Source/Core/NptNetwork.h \
      Neptune/Source/Core/NptQueue.h \
      Neptune/Source/Core/NptRingBuffer.h \
      Neptune/Source/Core/NptSimpleMessageQueue.h \
      Neptune/Source/Core/NptSockets.h \
      Neptune/Source/Core/NptStreams.h \
      Neptune/Source/Core/NptStrings.h \
      Neptune/Source/Core/NptSystem.h \
      Neptune/Source/Core/NptThreads.h \
      Neptune/Source/Core/NptTime.h \
      Neptune/Source/Core/NptUri.h \
      Neptune/Source/Core/NptUtils.h \
      Neptune/Source/Core/NptXml.h \
      Neptune/Source/Core/NptLogging.h \
      Neptune/Source/Core/NptResults.h \
      Neptune/Source/System/Bsd/NptBsdSockets.h \
      Neptune/Source/System/Bsd/NptBsdNetwork.h \
      Neptune/Source/System/Linux/NptLinuxNetwork.h \
      Neptune/Source/System/Posix/NptPosixSystem.h \
      Neptune/Source/System/Posix/NptSelectableMessageQueue.h \
      Neptune/Source/System/Posix/NptPosixQueue.h \
      Neptune/Source/System/Posix/NptPosixThreads.h \
      Neptune/Source/System/Posix/NptPosixFile.h \
      Neptune/Source/System/StdC/NptStdCTime.h \
      Neptune/Source/System/StdC/NptStdcDebug.h \
      Neptune/Source/System/StdC/NptStdcEnvironment.h \
      Platinum/Source/Devices/MediaRenderer/PltMediaRenderer.h \
      Platinum/Source/Devices/MediaRenderer/PltMediaController.h \
      Platinum/Source/Devices/MediaRenderer/AVTransportSCPD.h \
      Platinum/Source/Devices/MediaRenderer/RdrConnectionManagerSCPD.h \
      Platinum/Source/Devices/MediaRenderer/RenderingControlSCPD.h \
      Platinum/Source/Devices/MediaConnect/X_MS_MediaReceiverRegistrarSCPD.h \
      Platinum/Source/Devices/MediaConnect/PltMediaConnect.h 

