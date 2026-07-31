package dev.qcom.bandmenu

import android.util.Log

object AppLog {
    @Volatile
    var debugEnabled = false

    fun d(tag: String, msg: String) {
        if (debugEnabled) Log.d(tag, msg)
    }

    fun e(tag: String, msg: String) {
        Log.e(tag, msg)
    }

    fun e(tag: String, msg: String, tr: Throwable) {
        Log.e(tag, msg, tr)
    }

    fun w(tag: String, msg: String) {
        Log.w(tag, msg)
    }

    fun w(tag: String, msg: String, tr: Throwable) {
        Log.w(tag, msg, tr)
    }

    fun i(tag: String, msg: String) {
        Log.i(tag, msg)
    }
}
