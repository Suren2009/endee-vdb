# Android JNI Build

Endee can be built as an Android JNI shared library for `arm64-v8a`.

## Prerequisites

- CMake
- Android NDK r26 or newer
- `ANDROID_NDK_HOME` or `ANDROID_NDK_ROOT` pointing at the NDK

## Build

```bash
android/scripts/build-android-arm64-v8a.sh
```

The output library is:

```text
build/android-arm64-v8a/arm64-v8a/libendee.so
```

Copy it into an Android app module under:

```text
app/src/main/jniLibs/arm64-v8a/libendee.so
```

Copy the Java wrapper from:

```text
android/java/io/endee/ndd/EndeeNative.java
```

into the app source tree, preserving the `io.endee.ndd` package.

## JNI API

`EndeeNative` exposes:

- manager lifecycle through constructor and `close()`
- `createIndex`, `deleteIndex`, `getIndexInfoJson`, `listIndexesJson`
- `addVectors`, `getVectorJson`, `deleteVector`, `updateFilters`
- `searchJson`

Result-bearing APIs return JSON strings so Android clients can bind responses to
their own model classes. Simple index names are stored under the default Endee
user namespace; callers may also pass a fully-qualified `user/index` name.

The Android JNI target disables server backup/tar APIs and builds only the
embedded vector storage/search path needed by mobile clients.
