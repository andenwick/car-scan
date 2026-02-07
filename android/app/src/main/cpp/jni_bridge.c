/**
 * jni_bridge.c — JNI bridge between Android/Kotlin and the OBD-II C library.
 *
 * Each JNIEXPORT function maps to a native method in ObdNative.kt.
 * The JNI function naming convention is:
 *   Java_<package>_<class>_<method>
 *   = Java_com_carscan_app_obd_ObdNative_<method>
 */

#include <jni.h>
#include <obd/obd.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  ELM327 AT Commands
 *  These return static string literals from the C library.
 * ═══════════════════════════════════════════════════════════════════════════ */

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdReset(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_reset());
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdEchoOff(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_echo_off());
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdLinefeedOff(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_linefeed_off());
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdProtocolAuto(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_protocol_auto());
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdHeadersOn(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_headers_on());
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cmdHeadersOff(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    return (*env)->NewStringUTF(env, obd_elm327_cmd_headers_off());
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  Response Handling
 * ═══════════════════════════════════════════════════════════════════════════ */

JNIEXPORT jint JNICALL
Java_com_carscan_app_obd_ObdNative_classifyResponse(
    JNIEnv *env, jobject thiz, jstring response)
{
    const char *str;
    jint result;

    (void)thiz;
    str = (*env)->GetStringUTFChars(env, response, NULL);
    if (!str) return 0;  /* OOM — JVM already threw OutOfMemoryError */
    result = (jint)obd_elm327_classify_response(str);
    (*env)->ReleaseStringUTFChars(env, response, str);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_cleanResponse(
    JNIEnv *env, jobject thiz, jstring raw)
{
    const char *raw_str;
    char out[OBD_MAX_RESPONSE_LEN];
    obd_result_t r;

    (void)thiz;
    raw_str = (*env)->GetStringUTFChars(env, raw, NULL);
    if (!raw_str) return NULL;  /* OOM — JVM already threw OutOfMemoryError */
    r = obd_elm327_clean_response(raw_str, out, sizeof(out));
    (*env)->ReleaseStringUTFChars(env, raw, raw_str);

    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, out);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  PID / Sensor
 * ═══════════════════════════════════════════════════════════════════════════ */

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_buildPidRequest(
    JNIEnv *env, jobject thiz, jint mode, jint pid)
{
    char out[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    (void)thiz;
    r = obd_pid_build_request((uint8_t)mode, (uint8_t)pid, out, sizeof(out));
    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jobject JNICALL
Java_com_carscan_app_obd_ObdNative_decodeSensor(
    JNIEnv *env, jobject thiz, jstring cleaned_hex)
{
    const char *hex;
    obd_pid_response_t pid_resp;
    obd_sensor_value_t val;
    jclass cls;
    jmethodID ctor;

    (void)thiz;
    hex = (*env)->GetStringUTFChars(env, cleaned_hex, NULL);
    if (!hex) return NULL;  /* OOM — JVM already threw OutOfMemoryError */

    if (obd_pid_parse_response(hex, &pid_resp) != OBD_OK ||
        obd_sensor_decode(&pid_resp, &val) != OBD_OK) {
        (*env)->ReleaseStringUTFChars(env, cleaned_hex, hex);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, cleaned_hex, hex);

    /* Construct SensorValue(pid: Int, value: Float, name: String, unit: String) */
    cls = (*env)->FindClass(env, "com/carscan/app/obd/SensorValue");
    if (!cls) return NULL;  /* Class stripped by ProGuard or not found */
    ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(IFLjava/lang/String;Ljava/lang/String;)V");
    if (!ctor) return NULL;  /* Constructor not found */
    return (*env)->NewObject(env, cls, ctor,
        (jint)val.pid, val.value,
        (*env)->NewStringUTF(env, val.name),
        (*env)->NewStringUTF(env, val.unit));
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_getSensorName(
    JNIEnv *env, jobject thiz, jint pid)
{
    char name[32];
    obd_result_t r;

    (void)thiz;
    r = obd_sensor_get_name((uint8_t)pid, name, sizeof(name));
    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, name);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  DTC
 * ═══════════════════════════════════════════════════════════════════════════ */

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_buildDtcRequest(
    JNIEnv *env, jobject thiz)
{
    char out[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    (void)thiz;
    r = obd_dtc_build_request(out, sizeof(out));
    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jobjectArray JNICALL
Java_com_carscan_app_obd_ObdNative_parseDtcResponse(
    JNIEnv *env, jobject thiz, jstring cleaned_hex)
{
    const char *hex;
    obd_dtc_list_t list;
    obd_result_t r;
    jclass cls;
    jmethodID ctor;
    jobjectArray arr;
    size_t i;

    (void)thiz;
    hex = (*env)->GetStringUTFChars(env, cleaned_hex, NULL);
    if (!hex) return NULL;  /* OOM — JVM already threw OutOfMemoryError */
    r = obd_dtc_parse_response(hex, &list);
    (*env)->ReleaseStringUTFChars(env, cleaned_hex, hex);

    cls = (*env)->FindClass(env, "com/carscan/app/obd/DtcCode");
    if (!cls) return NULL;  /* Class stripped by ProGuard or not found */
    ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(IILjava/lang/String;)V");
    if (!ctor) return NULL;  /* Constructor not found */

    /* Return empty array on parse failure */
    if (r != OBD_OK) {
        return (*env)->NewObjectArray(env, 0, cls, NULL);
    }

    arr = (*env)->NewObjectArray(env, (jsize)list.count, cls, NULL);
    for (i = 0; i < list.count; i++) {
        jobject obj = (*env)->NewObject(env, cls, ctor,
            (jint)list.dtcs[i].category,
            (jint)list.dtcs[i].code,
            (*env)->NewStringUTF(env, list.dtcs[i].formatted));
        (*env)->SetObjectArrayElement(env, arr, (jsize)i, obj);
        (*env)->DeleteLocalRef(env, obj);
    }
    return arr;
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  VIN
 * ═══════════════════════════════════════════════════════════════════════════ */

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_buildVinRequest(
    JNIEnv *env, jobject thiz)
{
    char out[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    (void)thiz;
    r = obd_vin_build_request(out, sizeof(out));
    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jstring JNICALL
Java_com_carscan_app_obd_ObdNative_parseVinResponse(
    JNIEnv *env, jobject thiz, jstring cleaned_hex)
{
    const char *hex;
    char vin[OBD_VIN_LENGTH + 1];
    obd_result_t r;

    (void)thiz;
    hex = (*env)->GetStringUTFChars(env, cleaned_hex, NULL);
    if (!hex) return NULL;  /* OOM — JVM already threw OutOfMemoryError */
    r = obd_vin_parse_response(hex, vin, sizeof(vin));
    (*env)->ReleaseStringUTFChars(env, cleaned_hex, hex);

    if (r != OBD_OK) return NULL;
    return (*env)->NewStringUTF(env, vin);
}
