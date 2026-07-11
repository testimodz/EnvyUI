#ifndef JNISTUFF
#define JNISTUFF
#include <jni.h>
#include <string>
#include <imgui.h>

inline JavaVM* jvm = nullptr;
inline jobject Loader = nullptr;

static JNIEnv* GetEnv(bool* attached) {
    JNIEnv* env = nullptr;
    *attached = false;
    jint result = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, nullptr);
        *attached = true;
    }
    return env;
}

static jclass GetClass(JNIEnv* env, const char* className) {
    if (!Loader) return nullptr;
    jclass loaderClass = env->FindClass("java/lang/ClassLoader");
    if (!loaderClass) return nullptr;
    jstring jClassName = env->NewStringUTF(className);
    jclass cls = (jclass)env->CallObjectMethod(
        Loader,
        env->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"),
        jClassName
    );
    env->DeleteLocalRef(jClassName);
    env->DeleteLocalRef(loaderClass);
    return cls;
}

void GLES3JNI_nativeAddChar(JNIEnv*, jclass, jint codepoint) {
    ImGui::GetIO().AddInputCharacter((unsigned int)codepoint);
}

void GLES3JNI_nativeKeyEvent(JNIEnv*, jclass, jint keyCode) {
    ImGuiIO& io = ImGui::GetIO();
    switch (keyCode) {
        case 67:  io.AddKeyEvent(ImGuiKey_Backspace,   true); io.AddKeyEvent(ImGuiKey_Backspace,   false); break;
        case 66:  io.AddKeyEvent(ImGuiKey_Enter,       true); io.AddKeyEvent(ImGuiKey_Enter,       false); break;
        case 21:  io.AddKeyEvent(ImGuiKey_LeftArrow,   true); io.AddKeyEvent(ImGuiKey_LeftArrow,   false); break;
        case 22:  io.AddKeyEvent(ImGuiKey_RightArrow,  true); io.AddKeyEvent(ImGuiKey_RightArrow,  false); break;
        case 112: io.AddKeyEvent(ImGuiKey_Delete,      true); io.AddKeyEvent(ImGuiKey_Delete,      false); break;
        default: break;
    }
}

static bool RegisterHelperNatives(JNIEnv* env) {
    jclass cls = GetClass(env, "com.mxp.Helper");
    if (!cls) return false;

    JNINativeMethod methods[] = {
        { (char*)"nativeAddChar",  (char*)"(I)V", (void*)GLES3JNI_nativeAddChar  },
        { (char*)"nativeKeyEvent", (char*)"(I)V", (void*)GLES3JNI_nativeKeyEvent },
    };

    jint ret = env->RegisterNatives(cls, methods, 2);
    env->DeleteLocalRef(cls);
    return ret == 0;
}

static jobject GetCurrentActivity(JNIEnv* env) {
    jclass atClass = env->FindClass("android/app/ActivityThread");
    if (!atClass) return nullptr;
    jobject at = env->CallStaticObjectMethod(
        atClass,
        env->GetStaticMethodID(atClass, "currentActivityThread", "()Landroid/app/ActivityThread;")
    );
    jfieldID activitiesField = env->GetFieldID(atClass, "mActivities", "Landroid/util/ArrayMap;");
    jobject activities = env->GetObjectField(at, activitiesField);
    jclass arrayMapClass = env->FindClass("android/util/ArrayMap");
    jobject activityRecord = env->CallObjectMethod(
        activities,
        env->GetMethodID(arrayMapClass, "valueAt", "(I)Ljava/lang/Object;"),
        0
    );
    jobject activity = env->GetObjectField(
        activityRecord,
        env->GetFieldID(env->GetObjectClass(activityRecord), "activity", "Landroid/app/Activity;")
    );
    env->DeleteLocalRef(atClass);
    env->DeleteLocalRef(arrayMapClass);
    return activity;
}

static void LoadDex(const uint8_t* bytes, size_t size) {
    bool attached;
    JNIEnv* env = GetEnv(&attached);
    if (!env) return;

    jclass atClass = env->FindClass("android/app/ActivityThread");
    jobject activityThread = env->CallStaticObjectMethod(
        atClass,
        env->GetStaticMethodID(atClass, "currentActivityThread", "()Landroid/app/ActivityThread;")
    );
    jobject app = env->CallObjectMethod(
        activityThread,
        env->GetMethodID(atClass, "getApplication", "()Landroid/app/Application;")
    );
    jclass contextClass = env->FindClass("android/content/Context");
    jobject classLoader = env->CallObjectMethod(
        app,
        env->GetMethodID(contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;")
    );

    jclass byteBufferClass = env->FindClass("java/nio/ByteBuffer");
    jbyteArray arr = env->NewByteArray((jsize)size);
    env->SetByteArrayRegion(arr, 0, (jsize)size, (const jbyte*)bytes);
    jobject byteBuffer = env->CallStaticObjectMethod(
        byteBufferClass,
        env->GetStaticMethodID(byteBufferClass, "wrap", "([B)Ljava/nio/ByteBuffer;"),
        arr
    );

    jclass imclClass = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    jobject loader = env->NewObject(
        imclClass,
        env->GetMethodID(imclClass, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V"),
        byteBuffer,
        classLoader
    );
    if (!loader) {
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    Loader = env->NewGlobalRef(loader);

    jclass helperClass = GetClass(env, "com.mxp.Helper");
    if (!helperClass) {
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    jobject activity = GetCurrentActivity(env);
    if (activity) {
        env->CallStaticVoidMethod(
            helperClass,
            env->GetStaticMethodID(helperClass, "init", "(Landroid/app/Activity;)V"),
            activity
        );
    }

    RegisterHelperNatives(env);

    env->DeleteLocalRef(arr);
    env->DeleteLocalRef(helperClass);
    env->DeleteLocalRef(atClass);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(byteBufferClass);
    env->DeleteLocalRef(imclClass);
    if (attached) jvm->DetachCurrentThread();
}

static void HideScreen(bool hide) {
    if (!Loader) return;
    bool attached;
    JNIEnv* env = GetEnv(&attached);
    if (!env) return;

    jclass helperClass = GetClass(env, "com.mxp.Helper");
    if (!helperClass) {
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    env->CallStaticVoidMethod(
        helperClass,
        env->GetStaticMethodID(helperClass, "hideScreen", "(Z)V"),
        (jboolean)hide
    );

    env->DeleteLocalRef(helperClass);
    if (attached) jvm->DetachCurrentThread();
}

static void IsVpnActive(bool active) {
    if (!Loader) return;
    bool attached;
    JNIEnv* env = GetEnv(&attached);
    if (!env) return;

    jclass helperClass = GetClass(env, "com.mxp.Helper");
    if (!helperClass) {
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    env->CallStaticVoidMethod(
        helperClass,
        env->GetStaticMethodID(helperClass, "IsVpnActive", "(Z)V"),
        (jboolean)active
    );

    env->DeleteLocalRef(helperClass);
    if (attached) jvm->DetachCurrentThread();
}

static void ShowKeyboard(bool show) {
    if (!Loader) return;
    bool attached;
    JNIEnv* env = GetEnv(&attached);
    if (!env) return;

    jclass helperClass = GetClass(env, "com.mxp.Helper");
    if (!helperClass) {
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    env->CallStaticVoidMethod(
        helperClass,
        env->GetStaticMethodID(helperClass, "showSoftKeyboard", "(Z)V"),
        (jboolean)show
    );

    env->DeleteLocalRef(helperClass);
    if (attached) jvm->DetachCurrentThread();
}

void CopyToClipboard(const char* text) {
    if (!text || !jvm) return;

    JNIEnv* env;
    jvm->AttachCurrentThread(&env, NULL);

    // Get current ActivityThread and Application
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);

    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);

    // Get ClipboardManager
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring clipboardService = env->NewStringUTF("clipboard");
    jobject clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, clipboardService);

    // Prepare ClipData and set it to clipboard
    jclass clipDataClass = env->FindClass("android/content/ClipData");
    jmethodID newPlainTextMethod = env->GetStaticMethodID(clipDataClass, "newPlainText", 
        "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");

    jstring label = env->NewStringUTF("CopiedText");
    jstring textToCopy = env->NewStringUTF(text);
    jobject clipData = env->CallStaticObjectMethod(clipDataClass, newPlainTextMethod, label, textToCopy);

    jclass clipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    jmethodID setPrimaryClipMethod = env->GetMethodID(clipboardManagerClass, "setPrimaryClip", "(Landroid/content/ClipData;)V");
    env->CallVoidMethod(clipboardManager, setPrimaryClipMethod, clipData);

    // Cleanup
    env->DeleteLocalRef(label);
    env->DeleteLocalRef(textToCopy);
    env->DeleteLocalRef(clipData);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(clipboardService);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(sCurrentActivityThread);
}

const char *getClipboardText() {
    const char *result;
    JNIEnv *env;
    
    jvm->AttachCurrentThread(&env, NULL);
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
   
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    auto contextClass = env->FindClass("android/content/Context");
    auto getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
    auto str = env->NewStringUTF("clipboard");
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
  
    jclass ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");

    jclass CharSequenceClass = env->FindClass("java/lang/CharSequence");
    auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");

    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        result = env->GetStringUTFChars(str, 0);  
    }
    return result;
}


#endif JNISTUFF
