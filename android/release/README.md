# Endee VDB — Android library (v0.1.0)

Prebuilt AAR artifacts for the Endee on-device vector database (`arm64-v8a`).

| File | Description |
|------|-------------|
| `endee-vdb-release-0.1.0.aar` | Optimized release build (use in production apps) |
| `endee-vdb-debug-0.1.0.aar` | Debug build (symbols, no minification) |

Rebuild from the repo root:

```bash
# Release (default)
android/scripts/build-aar.sh

# Debug
android/scripts/build-aar.sh debug
```

```powershell
android\scripts\build-aar.ps1
android\scripts\build-aar.ps1 -Variant debug
```

---

## 1. Add the AAR to your app

Copy `endee-vdb-release-0.1.0.aar` into your app module, for example:

```text
app/libs/endee-vdb-release-0.1.0.aar
```

**Groovy (`app/build.gradle`):**

```gradle
dependencies {
    implementation files('libs/endee-vdb-release-0.1.0.aar')
}
```

**Kotlin DSL (`app/build.gradle.kts`):**

```kotlin
dependencies {
    implementation(files("libs/endee-vdb-release-0.1.0.aar"))
}
```

Sync Gradle. The AAR bundles `libendee.so` for **arm64-v8a** only; use a physical device or arm64 emulator.

---

## 2. Permissions and storage

Endee stores data on disk. Use app-private storage (no extra permission on modern Android):

```kotlin
val dataDir = File(context.filesDir, "endee").apply { mkdirs() }.absolutePath
```

---

## 3. Sample usage (Kotlin)

```kotlin
import io.endee.ndd.EndeeNative
import org.json.JSONArray
import org.json.JSONObject

class EndeeExample(private val context: Context) {

    fun run() {
        val dataDir = File(context.filesDir, "endee").apply { mkdirs() }.absolutePath

        EndeeNative(dataDir).use { db ->
            val indexName = "demo"
            val dimension = 4

            // Create index (cosine, float32 vectors)
            val created = db.createIndex(
                indexName,
                dimension,
                "cosine",
                "float32",
                10_000,
                16,
                200,
                0,
                ""
            )
            if (!created) error("createIndex failed")

            // Insert vectors
            val ids = arrayOf("doc-1", "doc-2")
            val vectors = arrayOf(
                floatArrayOf(1f, 0f, 0f, 0f),
                floatArrayOf(0.9f, 0.1f, 0f, 0f)
            )
            val metadata = arrayOf(byteArrayOf(), byteArrayOf())
            val filters = arrayOf<String?>(null, null)
            val norms = floatArrayOf(1f, 1f)

            if (!db.addVectors(indexName, ids, vectors, metadata, filters, norms)) {
                error("addVectors failed")
            }

            // k-NN search (results as JSON)
            val query = floatArrayOf(1f, 0f, 0f, 0f)
            val resultsJson = db.searchJson(
                indexName,
                query,
                k = 2,
                filterJson = "",
                includeVectors = false,
                ef = 100
            )
            val hits = JSONArray(resultsJson)
            for (i in 0 until hits.length()) {
                val hit = hits.getJSONObject(i)
                val id = hit.getString("id")
                val score = hit.getDouble("score")
                Log.d("Endee", "hit id=$id score=$score")
            }

            // Index metadata
            val infoJson = db.getIndexInfoJson(indexName)
            Log.d("Endee", "index info: $infoJson")

            // List indexes
            Log.d("Endee", "indexes: ${db.listIndexesJson()}")
        }
    }
}
```

Call `run()` from a background thread (disk I/O and native work should not block the UI thread).

---

## 4. Sample usage (Java)

```java
import android.content.Context;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;

import io.endee.ndd.EndeeNative;

public final class EndeeExample {

    public void run(Context context) throws Exception {
        File dir = new File(context.getFilesDir(), "endee");
        if (!dir.exists() && !dir.mkdirs()) {
            throw new IllegalStateException("Failed to create data directory");
        }
        String dataDir = dir.getAbsolutePath();

        try (EndeeNative db = new EndeeNative(dataDir)) {
            String indexName = "demo";
            int dimension = 4;

            if (!db.createIndex(indexName, dimension, "cosine", "float32",
                    10_000, 16, 200, 0, "")) {
                throw new IllegalStateException("createIndex failed");
            }

            String[] ids = {"doc-1", "doc-2"};
            float[][] vectors = {
                    {1f, 0f, 0f, 0f},
                    {0.9f, 0.1f, 0f, 0f}
            };
            byte[][] metadata = {new byte[0], new byte[0]};
            String[] filters = {null, null};
            float[] norms = {1f, 1f};

            if (!db.addVectors(indexName, ids, vectors, metadata, filters, norms)) {
                throw new IllegalStateException("addVectors failed");
            }

            float[] query = {1f, 0f, 0f, 0f};
            String resultsJson = db.searchJson(indexName, query, 2, "", false, 100);
            JSONArray hits = new JSONArray(resultsJson);
            for (int i = 0; i < hits.length(); i++) {
                JSONObject hit = hits.getJSONObject(i);
                Log.d("Endee", "hit id=" + hit.getString("id") + " score=" + hit.getDouble("score"));
            }
        }
    }
}
```

---

## 5. API overview

| Method | Description |
|--------|-------------|
| `EndeeNative(dataDir)` | Open/create database at `dataDir` |
| `createIndex(...)` | Create a vector index |
| `addVectors(...)` | Upsert vectors with optional metadata/filters |
| `searchJson(...)` | k-NN search; returns JSON array of hits |
| `getVectorJson` / `deleteVector` | Point read/delete |
| `getIndexInfoJson` / `listIndexesJson` | Index metadata |
| `updateFilters` | Update filter strings for existing ids |
| `close()` | Flush and release native resources (also called by `use` / try-with-resources) |

Search and info methods return **JSON strings** — parse with `org.json`, Moshi, Gson, or kotlinx.serialization.

---

## 6. Requirements

- **minSdk** 24
- **ABI** arm64-v8a
- **JDK** 11+ for the app module

For build instructions from source, see [docs/android.md](../../docs/android.md).
