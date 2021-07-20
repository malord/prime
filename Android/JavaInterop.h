// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ANDROID_JAVAINTEROP_H
#define PRIME_ANDROID_JAVAINTEROP_H

#include "../Config.h"
#include "../ThreadSpecificData.h"
#include <jni.h>
#include <string>

namespace Prime {

class JavaInterop {
public:
    /// Call this from your JNI_OnLoad.
    static void setVM(JavaVM* vm);

    static JavaVM* getVM();

    /// Construct a JavaInterop that borrows a JNIEnv from the VM.
    JavaInterop();

    /// Construct a JavaInterop that uses an existing JNIEnv which is guaranteed to exist for the lifetime of
    /// the JavaInterop.
    explicit JavaInterop(JNIEnv* env);

    JNIEnv* getEnv();

    std::string stringFromJava(jstring j);

    //
    // JavaClass
    //

    class PRIME_PUBLIC JavaClass {
    public:
        JavaClass();

        JavaClass(JNIEnv* env, const char* className);

        ~JavaClass();

        bool load(JNIEnv* env, const char* className);

        bool isLoaded() const { return _class != NULL; }

        JNIEnv* getEnv() const { return _env; }

        operator jclass() const { return _class; }

        jclass get() const { return _class; }

        void unload();

        jmethodID getStaticMethod(const char* methodName, const char* paramCode);

    private:
        operator void*() const { return NULL; }

        JNIEnv* _env;
        jclass _class;
    };

    //
    // StringFromJava
    //

    class PRIME_PUBLIC StringFromJava {
    public:
        StringFromJava(JNIEnv* env, jstring jString);

        ~StringFromJava();

        const char* c_str() const
        {
            PRIME_ASSERT(_string != NULL);
            return _string;
        }

        const char* getSafe() const { return _string ? _string : ""; }

    private:
        const char* _string;
        JNIEnv* _env;
        jstring _jString;
    };

    //
    // StringToJava
    //

    class PRIME_PUBLIC StringToJava {
    public:
        StringToJava();

        StringToJava(JNIEnv* env, const char* string);

        ~StringToJava();

        bool set(JNIEnv* env, const char* string);

        bool isSet() const { return _string != NULL; }

        jstring get() const { return _string; }

        operator jstring() const { return _string; }

        void clear();

    private:
        operator void*() const { return NULL; }

        JNIEnv* _env;
        jstring _string;
    };

private:
    JNIEnv* _env;

    static void threadDestructCallback(void*);

    static ThreadSpecificData* getThreadSpecificData();

    static JavaVM* _vm;
};

}

#endif
