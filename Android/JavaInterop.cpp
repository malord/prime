// Copyright 2000-2021 Mark H. P. Lord

#include "JavaInterop.h"

namespace Prime {

//
// JavaInterop::JavaClass
//

JavaInterop::JavaClass::JavaClass()
{
    _class = NULL;
}

JavaInterop::JavaClass::JavaClass(JNIEnv* env, const char* className)
{
    _class = NULL;
    load(env, className);
}

JavaInterop::JavaClass::~JavaClass()
{
    unload();
}

void JavaInterop::JavaClass::unload()
{
    if (_class) {
        _env->DeleteLocalRef(_class);
        _class = NULL;
    }
}

bool JavaInterop::JavaClass::load(JNIEnv* env, const char* className)
{
    unload();

    _env = env;

    _class = env->FindClass(className);
    if (!_class) {
        Trace("Failed to load Java class: %s.", className);
    }

    return _class;
}

jmethodID JavaInterop::JavaClass::getStaticMethod(const char* methodName, const char* paramCode)
{
    if (!_class) {
        return NULL;
    }

    jmethodID methodID = _env->GetStaticMethodID(_class, methodName, paramCode);
    if (!methodID) {
        Trace("Unable to find Java static method: %s.", methodName);
    }

    return methodID;
}

//
// JavaInterop::StringFromJava
//

JavaInterop::StringFromJava::StringFromJava(JNIEnv* env, jstring jString)
    : _env(env)
    , _jString(jString)
{
    if (jString) {
        _string = env->GetStringUTFChars(jString, NULL);
    } else {
        _string = NULL;
    }
}

JavaInterop::StringFromJava::~StringFromJava()
{
    if (_string) {
        _env->ReleaseStringUTFChars(_jString, _string);
    }
}

//
// JavaInterop::StringToJava
//

JavaInterop::StringToJava::StringToJava()
{
    _string = NULL;
}

JavaInterop::StringToJava::StringToJava(JNIEnv* env, const char* string)
{
    _string = NULL;
    set(env, string);
}

JavaInterop::StringToJava::~StringToJava()
{
    clear();
}

bool JavaInterop::StringToJava::set(JNIEnv* env, const char* string)
{
    clear();

    _env = env;
    _string = env->NewStringUTF(string);

    return _string != NULL;
}

void JavaInterop::StringToJava::clear()
{
    if (_string) {
        _env->DeleteLocalRef(_string);
        _string = NULL;
    }
}

//
// JavaInterop
//

JavaVM* JavaInterop::_vm = NULL;

ThreadSpecificData* JavaInterop::getThreadSpecificData()
{
    static ThreadSpecificData threadData(Log::getGlobal(), &JavaInterop::threadDestructCallback);
    return &threadData;
}

void JavaInterop::setVM(JavaVM* vm)
{
    _vm = vm;
}

JavaVM* JavaInterop::getVM()
{
    return _vm;
}

void JavaInterop::threadDestructCallback(void* data)
{
    if (JavaVM* vm = (JavaVM*)data) {
        vm->DetachCurrentThread();
    }
}

JavaInterop::JavaInterop()
    : _env(NULL)
{
}

JavaInterop::JavaInterop(JNIEnv* env)
    : _env(env)
{
}

JNIEnv* JavaInterop::getEnv()
{
    if (_env) {
        return _env;
    }

    if (!_vm) {
        return NULL;
    }

    JNIEnv* env;
    jint ret = _vm->GetEnv((void**)&env, JNI_VERSION_1_4);

    switch (ret) {
    case JNI_OK:
        _env = env;
        return env;

    case JNI_EDETACHED:
        if (_vm->AttachCurrentThread(&env, NULL) < 0) {
            DeveloperWarning("Unable to attach Java thread.");
            return NULL;
        }

        getThreadSpecificData()->set((void*)_vm);
        _env = env;
        return env;

    case JNI_EVERSION:
        RuntimeError("JNI 1.4 not supported.");
        return NULL;

    default:
        DeveloperWarning("Java GetEnv failed.");
        return NULL;
    }
}

std::string JavaInterop::stringFromJava(jstring j)
{
    std::string string;

    if (j) {
        const char* chars = getEnv()->GetStringUTFChars(j, NULL);
        if (chars) {
            string.assign(chars);
            getEnv()->ReleaseStringUTFChars(j, chars);
        }
    }

    return string;
}
}
