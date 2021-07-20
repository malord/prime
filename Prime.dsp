# Microsoft Developer Studio Project File - Name="Prime" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Prime - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Prime.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Prime.mak" CFG="Prime - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Prime - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Prime - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Prime - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Build\VC6\Release"
# PROP Intermediate_Dir "Build\VC6\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\zlib" /I "..\..\win-iconv" /I "..\..\sqlite3" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Prime - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Build\VC6\Debug"
# PROP Intermediate_Dir "Build\VC6\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\zlib" /I "..\..\win-iconv" /I "..\..\sqlite3" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Prime - Win32 Release"
# Name "Prime - Win32 Debug"
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Windows\InitialiseCOM.h
# End Source File
# Begin Source File

SOURCE=.\Windows\MessageBoxLog.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\MessageBoxLog.h
# End Source File
# Begin Source File

SOURCE=.\Windows\RegistrySettingsStore.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\RegistrySettingsStore.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsClock.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsCondition.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsCondition.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsConfig.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsCriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsDirectoryReader.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsDirectoryReader.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsDynamicLibrary.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsDynamicLibrary.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsEvent.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsFileLocations.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsFileStream.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsFileStream.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsWildcardExpansion.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsWildcardExpansion.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsLog.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsLog.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsMutex.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsMutex.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsNonRecursiveMutex.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsProcess.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsReadWriteLock.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsReadWriteLock.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSecureRNG.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSecureRNG.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSemaphore.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSocketSupport.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsSocketSupport.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsTerminationHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsTerminationHandler.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsThread.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsThread.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsThreadSpecificData.cpp
# End Source File
# Begin Source File

SOURCE=.\Windows\WindowsThreadSpecificData.h
# End Source File
# Begin Source File

SOURCE=.\Windows\WndLibLog.h
# End Source File
# End Group
# Begin Group "Emulated"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Emulated\EmulatedBarrier.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedBarrier.h
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedEvent.h
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedWildcardExpansion.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedWildcardExpansion.h
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedReadWriteLock.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedReadWriteLock.h
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedSemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulated\EmulatedSemaphore.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\2D.h
# End Source File
# Begin Source File

SOURCE=.\Adler32.h
# End Source File
# Begin Source File

SOURCE=.\ANSIEscapeParser.cpp
# End Source File
# Begin Source File

SOURCE=.\ANSIEscapeParser.h
# End Source File
# Begin Source File

SOURCE=.\ANSIEscapes.h
# End Source File
# Begin Source File

SOURCE=.\ANSILog.cpp
# End Source File
# Begin Source File

SOURCE=.\ANSILog.h
# End Source File
# Begin Source File

SOURCE=.\Array.h
# End Source File
# Begin Source File

SOURCE=.\ArrayView.h
# End Source File
# Begin Source File

SOURCE=.\ASCII.h
# End Source File
# Begin Source File

SOURCE=.\AtomicCounter.h
# End Source File
# Begin Source File

SOURCE=.\Barrier.h
# End Source File
# Begin Source File

SOURCE=.\Base64Decoder.cpp
# End Source File
# Begin Source File

SOURCE=.\Base64Decoder.h
# End Source File
# Begin Source File

SOURCE=.\Base64Encoder.cpp
# End Source File
# Begin Source File

SOURCE=.\Base64Encoder.h
# End Source File
# Begin Source File

SOURCE=.\BinaryPropertyListReader.cpp
# End Source File
# Begin Source File

SOURCE=.\BinaryPropertyListReader.h
# End Source File
# Begin Source File

SOURCE=.\BinaryPropertyListWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\BinaryPropertyListWriter.h
# End Source File
# Begin Source File

SOURCE=.\Callback.h
# End Source File
# Begin Source File

SOURCE=.\CallbackLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CallbackLog.h
# End Source File
# Begin Source File

SOURCE=.\CallBeforeMain.h
# End Source File
# Begin Source File

SOURCE=.\ChunkedReader.cpp
# End Source File
# Begin Source File

SOURCE=.\ChunkedReader.h
# End Source File
# Begin Source File

SOURCE=.\ChunkedWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\ChunkedWriter.h
# End Source File
# Begin Source File

SOURCE=.\CircularQueue.h
# End Source File
# Begin Source File

SOURCE=.\Clocks.h
# End Source File
# Begin Source File

SOURCE=.\Colour.h
# End Source File
# Begin Source File

SOURCE=.\CommandLineWildcardExpansion.h
# End Source File
# Begin Source File

SOURCE=.\CommandLineParser.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandLineParser.h
# End Source File
# Begin Source File

SOURCE=.\CommandLineRecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandLineRecoder.h
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\Condition.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\ConsoleLog.cpp
# End Source File
# Begin Source File

SOURCE=.\ConsoleLog.h
# End Source File
# Begin Source File

SOURCE=.\Convert.cpp
# End Source File
# Begin Source File

SOURCE=.\Convert.h
# End Source File
# Begin Source File

SOURCE=.\CPUCount.cpp
# End Source File
# Begin Source File

SOURCE=.\CPUCount.h
# End Source File
# Begin Source File

SOURCE=.\CPUUsage.h
# End Source File
# Begin Source File

SOURCE=.\CRC32.cpp
# End Source File
# Begin Source File

SOURCE=.\CRC32.h
# End Source File
# Begin Source File

SOURCE=.\CSVParser.cpp
# End Source File
# Begin Source File

SOURCE=.\CSVParser.h
# End Source File
# Begin Source File

SOURCE=.\CSVWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\CSVWriter.h
# End Source File
# Begin Source File

SOURCE=.\Data.cpp
# End Source File
# Begin Source File

SOURCE=.\Data.h
# End Source File
# Begin Source File

SOURCE=.\Database.cpp
# End Source File
# Begin Source File

SOURCE=.\Database.h
# End Source File
# Begin Source File

SOURCE=.\DateTime.cpp
# End Source File
# Begin Source File

SOURCE=.\DateTime.h
# End Source File
# Begin Source File

SOURCE=.\Decimal.cpp
# End Source File
# Begin Source File

SOURCE=.\Decimal.h
# End Source File
# Begin Source File

SOURCE=.\DefaultLog.h
# End Source File
# Begin Source File

SOURCE=.\StandardMemoryManager.cpp
# End Source File
# Begin Source File

SOURCE=.\StandardMemoryManager.h
# End Source File
# Begin Source File

SOURCE=.\DefaultTaskSystem.h
# End Source File
# Begin Source File

SOURCE=.\DeflateStream.cpp
# End Source File
# Begin Source File

SOURCE=.\DeflateStream.h
# End Source File
# Begin Source File

SOURCE=.\Dictionary.h
# End Source File
# Begin Source File

SOURCE=.\DictionarySettingsStore.cpp
# End Source File
# Begin Source File

SOURCE=.\DictionarySettingsStore.h
# End Source File
# Begin Source File

SOURCE=.\DirectHTTPConnectionFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectHTTPConnectionFactory.h
# End Source File
# Begin Source File

SOURCE=.\DirectoryLoader.h
# End Source File
# Begin Source File

SOURCE=.\DirectoryReader.h
# End Source File
# Begin Source File

SOURCE=.\DirectoryReaderBase.h
# End Source File
# Begin Source File

SOURCE=.\DJB2Hash.h
# End Source File
# Begin Source File

SOURCE=.\DoubleLinkList.h
# End Source File
# Begin Source File

SOURCE=.\DowngradeLog.cpp
# End Source File
# Begin Source File

SOURCE=.\DowngradeLog.h
# End Source File
# Begin Source File

SOURCE=.\DynamicLibrary.h
# End Source File
# Begin Source File

SOURCE=.\ByteOrder.cpp
# End Source File
# Begin Source File

SOURCE=.\ByteOrder.h
# End Source File
# Begin Source File

SOURCE=.\Event.h
# End Source File
# Begin Source File

SOURCE=.\File.cpp
# End Source File
# Begin Source File

SOURCE=.\File.h
# End Source File
# Begin Source File

SOURCE=.\FileLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\FileLoader.h
# End Source File
# Begin Source File

SOURCE=.\FileLocations.cpp
# End Source File
# Begin Source File

SOURCE=.\FileLocations.h
# End Source File
# Begin Source File

SOURCE=.\FileLog.cpp
# End Source File
# Begin Source File

SOURCE=.\FileLog.h
# End Source File
# Begin Source File

SOURCE=.\PropertyListFileWriter.h
# End Source File
# Begin Source File

SOURCE=.\FileProperties.h
# End Source File
# Begin Source File

SOURCE=.\FileSettingsStore.h
# End Source File
# Begin Source File

SOURCE=.\FileStream.h
# End Source File
# Begin Source File

SOURCE=.\FileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSystem.h
# End Source File
# Begin Source File

SOURCE=.\Fletcher8.h
# End Source File
# Begin Source File

SOURCE=.\FrameTimer.h
# End Source File
# Begin Source File

SOURCE=.\WildcardExpansionBase.h
# End Source File
# Begin Source File

SOURCE=.\WildcardExpansion.h
# End Source File
# Begin Source File

SOURCE=.\WildcardExpansionLoader.h
# End Source File
# Begin Source File

SOURCE=.\GZipFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipFormat.h
# End Source File
# Begin Source File

SOURCE=.\GZipWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipWriter.h
# End Source File
# Begin Source File

SOURCE=.\Hasher.cpp
# End Source File
# Begin Source File

SOURCE=.\Hasher.h
# End Source File
# Begin Source File

SOURCE=.\HashStream.h
# End Source File
# Begin Source File

SOURCE=.\Prime.dsp
# End Source File
# Begin Source File

SOURCE=.\Prime.vcxproj
# End Source File
# Begin Source File

SOURCE=.\Prime.vcxproj.filters
# End Source File
# Begin Source File

SOURCE=.\HTTPConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPConnection.h
# End Source File
# Begin Source File

SOURCE=.\HTTPCookie.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPCookie.h
# End Source File
# Begin Source File

SOURCE=.\HTTPFileServer.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPFileServer.h
# End Source File
# Begin Source File

SOURCE=.\HTTPHeaders.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPHeaders.h
# End Source File
# Begin Source File

SOURCE=.\HTTPMultiSocketServer.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPMultiSocketServer.h
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.h
# End Source File
# Begin Source File

SOURCE=.\HTTPSettingsSessionManager.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPSettingsSessionManager.h
# End Source File
# Begin Source File

SOURCE=.\HTTPSocketServer.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPSocketServer.h
# End Source File
# Begin Source File

SOURCE=.\IconvReader.cpp
# End Source File
# Begin Source File

SOURCE=.\IconvReader.h
# End Source File
# Begin Source File

SOURCE=.\IconvWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\IconvWrapper.h
# End Source File
# Begin Source File

SOURCE=.\ImpliedComparisonOperators.h
# End Source File
# Begin Source File

SOURCE=.\InflateStream.cpp
# End Source File
# Begin Source File

SOURCE=.\InflateStream.h
# End Source File
# Begin Source File

SOURCE=.\JSONReader.cpp
# End Source File
# Begin Source File

SOURCE=.\JSONReader.h
# End Source File
# Begin Source File

SOURCE=.\JSONWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\JSONWriter.h
# End Source File
# Begin Source File

SOURCE=.\Lexer.cpp
# End Source File
# Begin Source File

SOURCE=.\Lexer.h
# End Source File
# Begin Source File

SOURCE=.\LICENSE.txt
# End Source File
# Begin Source File

SOURCE=.\Lock.h
# End Source File
# Begin Source File

SOURCE=.\Log.cpp
# End Source File
# Begin Source File

SOURCE=.\Log.h
# End Source File
# Begin Source File

SOURCE=.\LoggingFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\LoggingFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\LogRecorder.cpp
# End Source File
# Begin Source File

SOURCE=.\LogRecorder.h
# End Source File
# Begin Source File

SOURCE=.\LogThreader.cpp
# End Source File
# Begin Source File

SOURCE=.\LogThreader.h
# End Source File
# Begin Source File

SOURCE=.\MD5.cpp
# End Source File
# Begin Source File

SOURCE=.\MD5.h
# End Source File
# Begin Source File

SOURCE=.\MemoryLeakDetector.h
# End Source File
# Begin Source File

SOURCE=.\MemoryManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MemoryManager.h
# End Source File
# Begin Source File

SOURCE=.\MersenneTwister.h
# End Source File
# Begin Source File

SOURCE=.\MIMETypes.cpp
# End Source File
# Begin Source File

SOURCE=.\MIMETypes.h
# End Source File
# Begin Source File

SOURCE=.\MultiFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\MultiLog.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiLog.h
# End Source File
# Begin Source File

SOURCE=.\MultiStream.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiStream.h
# End Source File
# Begin Source File

SOURCE=.\Mutex.h
# End Source File
# Begin Source File

SOURCE=.\MySQLDatabase.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\MySQLDatabase.h
# End Source File
# Begin Source File

SOURCE=.\NetworkStream.cpp
# End Source File
# Begin Source File

SOURCE=.\NetworkStream.h
# End Source File
# Begin Source File

SOURCE=.\NotNull.h
# End Source File
# Begin Source File

SOURCE=.\NumberUtils.h
# End Source File
# Begin Source File

SOURCE=.\OneAtATimeHash.h
# End Source File
# Begin Source File

SOURCE=.\Oniguruma.h
# End Source File
# Begin Source File

SOURCE=.\OpenMode.h
# End Source File
# Begin Source File

SOURCE=.\Optional.h
# End Source File
# Begin Source File

SOURCE=.\ORM.cpp
# End Source File
# Begin Source File

SOURCE=.\ORM.h
# End Source File
# Begin Source File

SOURCE=.\ParkMillerRNG.h
# End Source File
# Begin Source File

SOURCE=.\NumberParsing.cpp
# End Source File
# Begin Source File

SOURCE=.\NumberParsing.h
# End Source File
# Begin Source File

SOURCE=.\Path.cpp
# End Source File
# Begin Source File

SOURCE=.\Path.h
# End Source File
# Begin Source File

SOURCE=.\Platform.h
# End Source File
# Begin Source File

SOURCE=.\PropertyListReader.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyListReader.h
# End Source File
# Begin Source File

SOURCE=.\PropertyListWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyListWriter.h
# End Source File
# Begin Source File

SOURCE=.\Precompile.cpp
# End Source File
# Begin Source File

SOURCE=.\Precompile.h
# End Source File
# Begin Source File

SOURCE=.\PrefixFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\PrefixFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\PrefixLog.cpp
# End Source File
# Begin Source File

SOURCE=.\PrefixLog.h
# End Source File
# Begin Source File

SOURCE=.\ProcessBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessBase.h
# End Source File
# Begin Source File

SOURCE=.\Processes.h
# End Source File
# Begin Source File

SOURCE=.\QuotedPrintableDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\QuotedPrintableDecoder.h
# End Source File
# Begin Source File

SOURCE=.\QuotedPrintableEncoder.cpp
# End Source File
# Begin Source File

SOURCE=.\QuotedPrintableEncoder.h
# End Source File
# Begin Source File

SOURCE=.\Range.h
# End Source File
# Begin Source File

SOURCE=.\RateLimiter.cpp
# End Source File
# Begin Source File

SOURCE=.\RateLimiter.h
# End Source File
# Begin Source File

SOURCE=.\ReadWriteLock.h
# End Source File
# Begin Source File

SOURCE=.\RefCounting.cpp
# End Source File
# Begin Source File

SOURCE=.\RefCounting.h
# End Source File
# Begin Source File

SOURCE=.\RefPtr.h
# End Source File
# Begin Source File

SOURCE=.\Regex.h
# End Source File
# Begin Source File

SOURCE=.\RNGBase.h
# End Source File
# Begin Source File

SOURCE=.\ScopedLock.h
# End Source File
# Begin Source File

SOURCE=.\ScopedPtr.h
# End Source File
# Begin Source File

SOURCE=.\SDBMHash.h
# End Source File
# Begin Source File

SOURCE=.\SecureRNG.h
# End Source File
# Begin Source File

SOURCE=.\SeekAvoidingStream.cpp
# End Source File
# Begin Source File

SOURCE=.\SeekAvoidingStream.h
# End Source File
# Begin Source File

SOURCE=.\Semaphore.h
# End Source File
# Begin Source File

SOURCE=.\Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Settings.h
# End Source File
# Begin Source File

SOURCE=.\SHA1.cpp
# End Source File
# Begin Source File

SOURCE=.\SHA1.h
# End Source File
# Begin Source File

SOURCE=.\SHA256.cpp
# End Source File
# Begin Source File

SOURCE=.\SHA256.h
# End Source File
# Begin Source File

SOURCE=.\SharedPtr.h
# End Source File
# Begin Source File

SOURCE=.\SignalSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\SignalSocket.h
# End Source File
# Begin Source File

SOURCE=.\SimpleLog.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleLog.h
# End Source File
# Begin Source File

SOURCE=.\SMTPConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\SMTPConnection.h
# End Source File
# Begin Source File

SOURCE=.\Socket.cpp
# End Source File
# Begin Source File

SOURCE=.\Socket.h
# End Source File
# Begin Source File

SOURCE=.\SocketAddress.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketAddress.h
# End Source File
# Begin Source File

SOURCE=.\SocketAddressParser.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketAddressParser.h
# End Source File
# Begin Source File

SOURCE=.\SocketListener.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketListener.h
# End Source File
# Begin Source File

SOURCE=.\SocketStream.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketStream.h
# End Source File
# Begin Source File

SOURCE=.\SocketSupport.h
# End Source File
# Begin Source File

SOURCE=.\Spew.h
# End Source File
# Begin Source File

SOURCE=.\Spew2.h
# End Source File
# Begin Source File

SOURCE=.\SQLiteDatabase.cpp
# End Source File
# Begin Source File

SOURCE=.\SQLiteDatabase.h
# End Source File
# Begin Source File

SOURCE=.\SSLContext.cpp
# End Source File
# Begin Source File

SOURCE=.\SSLContext.h
# End Source File
# Begin Source File

SOURCE=.\OpenSSLDirectHTTPConnectionFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenSSLDirectHTTPConnectionFactory.h
# End Source File
# Begin Source File

SOURCE=.\SSLStream.cpp
# End Source File
# Begin Source File

SOURCE=.\SSLStream.h
# End Source File
# Begin Source File

SOURCE=.\SSLSupport.cpp
# End Source File
# Begin Source File

SOURCE=.\SSLSupport.h
# End Source File
# Begin Source File

SOURCE=.\StandardApp.cpp
# End Source File
# Begin Source File

SOURCE=.\StandardApp.h
# End Source File
# Begin Source File

SOURCE=.\StandardResponseFileLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\StandardResponseFileLoader.h
# End Source File
# Begin Source File

SOURCE=.\StdioLog.cpp
# End Source File
# Begin Source File

SOURCE=.\StdioLog.h
# End Source File
# Begin Source File

SOURCE=.\StdioStream.cpp
# End Source File
# Begin Source File

SOURCE=.\StdioStream.h
# End Source File
# Begin Source File

SOURCE=.\StdioUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\StdioUtils.h
# End Source File
# Begin Source File

SOURCE=.\Stopwatch.h
# End Source File
# Begin Source File

SOURCE=.\Stream.cpp
# End Source File
# Begin Source File

SOURCE=.\Stream.h
# End Source File
# Begin Source File

SOURCE=.\StreamBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamBuffer.h
# End Source File
# Begin Source File

SOURCE=.\StreamLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamLoader.h
# End Source File
# Begin Source File

SOURCE=.\StreamLoaderTask.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamLoaderTask.h
# End Source File
# Begin Source File

SOURCE=.\StreamLog.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamLog.h
# End Source File
# Begin Source File

SOURCE=.\StringUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtils.h
# End Source File
# Begin Source File

SOURCE=.\StringSearching.h
# End Source File
# Begin Source File

SOURCE=.\StringStream.cpp
# End Source File
# Begin Source File

SOURCE=.\StringStream.h
# End Source File
# Begin Source File

SOURCE=.\StringView.h
# End Source File
# Begin Source File

SOURCE=.\Substream.cpp
# End Source File
# Begin Source File

SOURCE=.\Substream.h
# End Source File
# Begin Source File

SOURCE=.\SystemFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\Task.cpp
# End Source File
# Begin Source File

SOURCE=.\Task.h
# End Source File
# Begin Source File

SOURCE=.\TaskQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskQueue.h
# End Source File
# Begin Source File

SOURCE=.\TaskSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskSystem.h
# End Source File
# Begin Source File

SOURCE=.\TaskSystemSelector.h
# End Source File
# Begin Source File

SOURCE=.\TempDirectory.cpp
# End Source File
# Begin Source File

SOURCE=.\TempDirectory.h
# End Source File
# Begin Source File

SOURCE=.\TempFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TempFile.h
# End Source File
# Begin Source File

SOURCE=.\Templates.h
# End Source File
# Begin Source File

SOURCE=.\TerminationHandler.h
# End Source File
# Begin Source File

SOURCE=.\TextEncoding.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEncoding.h
# End Source File
# Begin Source File

SOURCE=.\TextLog.cpp
# End Source File
# Begin Source File

SOURCE=.\TextLog.h
# End Source File
# Begin Source File

SOURCE=.\TextReader.cpp
# End Source File
# Begin Source File

SOURCE=.\TextReader.h
# End Source File
# Begin Source File

SOURCE=.\Thread.h
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.h
# End Source File
# Begin Source File

SOURCE=.\ThreadPoolTaskSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadPoolTaskSystem.h
# End Source File
# Begin Source File

SOURCE=.\ThreadSafeStream.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadSafeStream.h
# End Source File
# Begin Source File

SOURCE=.\ThreadSpecificData.h
# End Source File
# Begin Source File

SOURCE=.\Timeout.h
# End Source File
# Begin Source File

SOURCE=.\Timespec.cpp
# End Source File
# Begin Source File

SOURCE=.\Timespec.h
# End Source File
# Begin Source File

SOURCE=.\UID.h
# End Source File
# Begin Source File

SOURCE=.\UIDCast.h
# End Source File
# Begin Source File

SOURCE=.\UnclosableStream.cpp
# End Source File
# Begin Source File

SOURCE=.\UnclosableStream.h
# End Source File
# Begin Source File

SOURCE=.\UnrolledArrays.h
# End Source File
# Begin Source File

SOURCE=.\URL.cpp
# End Source File
# Begin Source File

SOURCE=.\URL.h
# End Source File
# Begin Source File

SOURCE=.\Value.cpp
# End Source File
# Begin Source File

SOURCE=.\Value.h
# End Source File
# Begin Source File

SOURCE=.\XMLNode.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLNode.h
# End Source File
# Begin Source File

SOURCE=.\XMLNodeReader.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLNodeReader.h
# End Source File
# Begin Source File

SOURCE=.\XMLNodeWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLNodeWriter.h
# End Source File
# Begin Source File

SOURCE=.\XMLPropertyListReader.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLPropertyListReader.h
# End Source File
# Begin Source File

SOURCE=.\XMLPropertyListWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLPropertyListWriter.h
# End Source File
# Begin Source File

SOURCE=.\XMLPullParser.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLPullParser.h
# End Source File
# Begin Source File

SOURCE=.\XMLWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLWriter.h
# End Source File
# Begin Source File

SOURCE=.\ZipFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\ZipFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipFormat.h
# End Source File
# Begin Source File

SOURCE=.\ZipOrDirectoryFileSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipOrDirectoryFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\ZipReader.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipReader.h
# End Source File
# Begin Source File

SOURCE=.\ZipWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipWriter.h
# End Source File
# End Target
# End Project
