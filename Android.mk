LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	UTF8RewindSupport.cpp \
	URL.cpp \
	HTTPParser.cpp \
	Common.cpp \
	Convert.cpp \
	Data.cpp \
	DateTime.cpp \
	File.cpp \
	FileSystem.cpp \
	JSONReader.cpp \
	JSONWriter.cpp \
	Lexer.cpp \
	Log.cpp \
	NumberParsing.cpp \
	Path.cpp \
	PrefixLog.cpp \
	Pthreads/PthreadsCondition.cpp \
	Pthreads/PthreadsMutex.cpp \
	Pthreads/PthreadsReadWriteLock.cpp \
	Pthreads/PthreadsRecursiveTimedMutex.cpp \
	Pthreads/PthreadsSemaphore.cpp \
	Pthreads/PthreadsThread.cpp \
	Pthreads/PthreadsThreadSpecificData.cpp \
	Pthreads/PthreadsTime.cpp \
	RefCounting.cpp \
	StdioStream.cpp \
	StdioUtils.cpp \
	Stream.cpp \
	StreamBuffer.cpp \
	StringUtils.cpp \
	StringStream.cpp \
	SystemFileSystem.cpp \
	TempFile.cpp \
	TextEncoding.cpp \
	TextReader.cpp \
	UnixTime.cpp \
	Unix/UnixClock.cpp \
	Unix/UnixCloseOnExec.cpp \
	Unix/UnixDirectoryReader.cpp \
	Unix/UnixFile.cpp \
	Unix/UnixFileProperties.cpp \
	Unix/UnixFileStream.cpp \
	Value.cpp \
	MultiFileSystem.cpp \
	CRC32.cpp \
	InflateStream.cpp \
	Substream.cpp \
	ZipFileSystem.cpp \
	PrefixFileSystem.cpp \
	ZipReader.cpp \
	ZipFormat.cpp \
	ZiporDirectoryFileSystem.cpp \
	Android/AndroidLog.cpp \
	Android/JavaInterop.cpp \
	XMLPullParser.cpp \
	XMLExpat.cpp \
	StreamLoader.cpp


LOCAL_CFLAGS += -iquote $(LOCAL_PATH) -iquote $(LOCAL_PATH)/../utf8rewind/include
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays

LOCAL_MODULE:= libPrime

include $(BUILD_STATIC_LIBRARY)
