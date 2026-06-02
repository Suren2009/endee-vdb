#include <jni.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/ndd.hpp"
#include "json/nlohmann_json.hpp"
#include "quant/common.hpp"

namespace {

struct NativeHandle {
    explicit NativeHandle(const std::string& data_dir, const PersistenceConfig& config) :
        manager(data_dir, config) {}

    IndexManager manager;
};

void throwJavaException(JNIEnv* env, const char* class_name, const std::string& message) {
    jclass cls = env->FindClass(class_name);
    if(cls != nullptr) {
        env->ThrowNew(cls, message.c_str());
    }
}

void throwIllegalArgument(JNIEnv* env, const std::string& message) {
    throwJavaException(env, "java/lang/IllegalArgumentException", message);
}

void throwIllegalState(JNIEnv* env, const std::string& message) {
    throwJavaException(env, "java/lang/IllegalStateException", message);
}

NativeHandle* requireHandle(JNIEnv* env, jlong handle) {
    if(handle == 0) {
        throwIllegalState(env, "Endee native manager handle is closed or null");
        return nullptr;
    }
    return reinterpret_cast<NativeHandle*>(handle);
}

std::string toString(JNIEnv* env, jstring value, const char* field_name, bool allow_null = false) {
    if(value == nullptr) {
        if(allow_null) {
            return {};
        }
        throwIllegalArgument(env, std::string(field_name) + " must not be null");
        return {};
    }

    const char* chars = env->GetStringUTFChars(value, nullptr);
    if(chars == nullptr) {
        throwIllegalState(env, std::string("Failed to read ") + field_name);
        return {};
    }

    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

std::string fullIndexId(const std::string& index_name) {
    if(index_name.empty()) {
        throw std::invalid_argument("indexName must not be empty");
    }
    if(index_name.find('/') != std::string::npos) {
        return index_name;
    }
    return settings::DEFAULT_USERNAME + "/" + index_name;
}

std::vector<float> toFloatVector(JNIEnv* env, jfloatArray array, const char* field_name) {
    if(array == nullptr) {
        throwIllegalArgument(env, std::string(field_name) + " must not be null");
        return {};
    }

    jsize length = env->GetArrayLength(array);
    std::vector<float> result(static_cast<size_t>(length));
    if(length > 0) {
        env->GetFloatArrayRegion(array, 0, length, result.data());
    }
    return result;
}

std::vector<uint8_t> toByteVector(JNIEnv* env, jbyteArray array) {
    if(array == nullptr) {
        return {};
    }

    jsize length = env->GetArrayLength(array);
    std::vector<uint8_t> result(static_cast<size_t>(length));
    if(length > 0) {
        env->GetByteArrayRegion(array, 0, length, reinterpret_cast<jbyte*>(result.data()));
    }
    return result;
}

float vectorNorm(const std::vector<float>& values) {
    double sum = 0.0;
    for(float value : values) {
        sum += static_cast<double>(value) * static_cast<double>(value);
    }
    return static_cast<float>(std::sqrt(sum));
}

nlohmann::json indexInfoToJson(const IndexInfo& info) {
    return {
            {"total_elements", static_cast<uint64_t>(info.total_elements)},
            {"dimension", static_cast<uint64_t>(info.dimension)},
            {"sparse_model", ndd::sparseScoringModelToString(info.sparse_model)},
            {"space_type", info.space_type_str},
            {"precision", ndd::quant::quantLevelToString(info.quant_level)},
            {"checksum", info.checksum},
            {"M", static_cast<uint64_t>(info.M)},
            {"ef_con", static_cast<uint64_t>(info.ef_con)}};
}

nlohmann::json resultToJson(const ndd::VectorResult& result) {
    nlohmann::json item = {
            {"similarity", result.similarity},
            {"id", result.id},
            {"meta", result.meta},
            {"filter", result.filter},
            {"norm", result.norm}};
    if(!result.vector.empty()) {
        item["vector"] = result.vector;
    }
    return item;
}

jstring toJString(JNIEnv* env, const nlohmann::json& value) {
    const std::string dumped = value.dump();
    return env->NewStringUTF(dumped.c_str());
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_io_endee_ndd_EndeeNative_createManager(JNIEnv* env,
                                            jclass,
                                            jstring data_dir,
                                            jint save_every_n_updates,
                                            jint save_interval_minutes,
                                            jboolean save_on_shutdown) {
    try {
        const std::string data_dir_value = toString(env, data_dir, "dataDir");
        if(env->ExceptionCheck()) {
            return 0;
        }

        PersistenceConfig config;
        if(save_every_n_updates > 0) {
            config.save_every_n_updates = static_cast<size_t>(save_every_n_updates);
        }
        if(save_interval_minutes > 0) {
            config.save_interval = std::chrono::minutes(save_interval_minutes);
        }
        config.save_on_shutdown = save_on_shutdown == JNI_TRUE;

        auto handle = std::make_unique<NativeHandle>(data_dir_value, config);
        return reinterpret_cast<jlong>(handle.release());
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return 0;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_io_endee_ndd_EndeeNative_destroyManager(JNIEnv* env, jclass, jlong handle) {
    try {
        delete reinterpret_cast<NativeHandle*>(handle);
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_endee_ndd_EndeeNative_createIndex(JNIEnv* env,
                                          jclass,
                                          jlong handle,
                                          jstring index_name,
                                          jint dim,
                                          jstring space_type,
                                          jstring precision,
                                          jint max_elements,
                                          jint m,
                                          jint ef_construction,
                                          jint checksum,
                                          jstring sparse_model) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return JNI_FALSE;
        }

        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        const std::string space_value = toString(env, space_type, "spaceType");
        std::string precision_value = toString(env, precision, "precision", true);
        std::string sparse_model_value = toString(env, sparse_model, "sparseModel", true);
        if(env->ExceptionCheck()) {
            return JNI_FALSE;
        }

        if(precision_value.empty()) {
            precision_value = "int8";
        } else if(precision_value == "int8d") {
            precision_value = "int8";
        } else if(precision_value == "int16d") {
            precision_value = "int16";
        }

        if(sparse_model_value.empty()) {
            sparse_model_value = "None";
        }

        auto quant_level = ndd::quant::stringToQuantLevel(precision_value);
        if(quant_level == ndd::quant::QuantizationLevel::UNKNOWN) {
            throw std::invalid_argument("Unsupported precision: " + precision_value);
        }

        auto sparse = ndd::sparseScoringModelFromString(sparse_model_value);
        if(!sparse.has_value()) {
            throw std::invalid_argument("Unsupported sparse model: " + sparse_model_value);
        }

        IndexConfig config{
                static_cast<size_t>(dim),
                *sparse,
                max_elements > 0 ? static_cast<size_t>(max_elements) : settings::DEFAULT_MAX_ELEMENTS,
                space_value,
                m > 0 ? static_cast<size_t>(m) : settings::DEFAULT_M,
                ef_construction > 0 ? static_cast<size_t>(ef_construction)
                                    : settings::DEFAULT_EF_CONSTRUCT,
                quant_level,
                checksum};

        return native->manager.createIndex(index_id, config) ? JNI_TRUE : JNI_FALSE;
    } catch(const std::invalid_argument& e) {
        throwIllegalArgument(env, e.what());
        return JNI_FALSE;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_endee_ndd_EndeeNative_deleteIndex(JNIEnv* env, jclass, jlong handle, jstring index_name) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return JNI_FALSE;
        }
        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        return native->manager.deleteIndex(index_id) ? JNI_TRUE : JNI_FALSE;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_endee_ndd_EndeeNative_getIndexInfoJson(JNIEnv* env,
                                               jclass,
                                               jlong handle,
                                               jstring index_name) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return nullptr;
        }
        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        auto info = native->manager.getIndexInfo(index_id);
        if(!info.has_value()) {
            return toJString(env, nullptr);
        }
        return toJString(env, indexInfoToJson(*info));
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return nullptr;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_endee_ndd_EndeeNative_listIndexesJson(JNIEnv* env, jclass, jlong handle) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return nullptr;
        }

        nlohmann::json items = nlohmann::json::array();
        for(const auto& [name, metadata] : native->manager.listUserIndexes(settings::DEFAULT_USERNAME)) {
            items.push_back({
                    {"name", name},
                    {"total_elements", static_cast<uint64_t>(metadata.total_elements)},
                    {"dimension", static_cast<uint64_t>(metadata.dimension)},
                    {"sparse_model", ndd::sparseScoringModelToString(metadata.sparse_model)},
                    {"space_type", metadata.space_type_str},
                    {"precision", ndd::quant::quantLevelToString(metadata.quant_level)},
                    {"checksum", metadata.checksum},
                    {"M", static_cast<uint64_t>(metadata.M)},
                    {"created_at",
                     static_cast<int64_t>(
                             std::chrono::system_clock::to_time_t(metadata.created_at))}});
        }
        return toJString(env, items);
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return nullptr;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_endee_ndd_EndeeNative_addVectors(JNIEnv* env,
                                         jclass,
                                         jlong handle,
                                         jstring index_name,
                                         jobjectArray ids,
                                         jobjectArray vectors,
                                         jobjectArray metadata,
                                         jobjectArray filters,
                                         jfloatArray norms) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return JNI_FALSE;
        }
        if(ids == nullptr || vectors == nullptr) {
            throw std::invalid_argument("ids and vectors must not be null");
        }

        const jsize count = env->GetArrayLength(ids);
        if(env->GetArrayLength(vectors) != count) {
            throw std::invalid_argument("ids and vectors must have the same length");
        }
        if(metadata != nullptr && env->GetArrayLength(metadata) != count) {
            throw std::invalid_argument("metadata must be null or match ids length");
        }
        if(filters != nullptr && env->GetArrayLength(filters) != count) {
            throw std::invalid_argument("filters must be null or match ids length");
        }
        if(norms != nullptr && env->GetArrayLength(norms) != count) {
            throw std::invalid_argument("norms must be null or match ids length");
        }

        std::vector<float> norms_vector;
        if(norms != nullptr) {
            norms_vector = toFloatVector(env, norms, "norms");
        }

        std::vector<ndd::VectorObject> batch;
        batch.reserve(static_cast<size_t>(count));

        for(jsize i = 0; i < count; ++i) {
            auto id = static_cast<jstring>(env->GetObjectArrayElement(ids, i));
            auto vector_array = static_cast<jfloatArray>(env->GetObjectArrayElement(vectors, i));
            auto meta_array = metadata == nullptr
                                      ? nullptr
                                      : static_cast<jbyteArray>(env->GetObjectArrayElement(metadata, i));
            auto filter = filters == nullptr
                                  ? nullptr
                                  : static_cast<jstring>(env->GetObjectArrayElement(filters, i));

            ndd::VectorObject item;
            item.id = toString(env, id, "ids[]");
            item.vector = toFloatVector(env, vector_array, "vectors[]");
            item.meta = toByteVector(env, meta_array);
            item.filter = toString(env, filter, "filters[]", true);
            item.norm = norms == nullptr ? vectorNorm(item.vector) : norms_vector[static_cast<size_t>(i)];

            env->DeleteLocalRef(id);
            env->DeleteLocalRef(vector_array);
            if(meta_array != nullptr) {
                env->DeleteLocalRef(meta_array);
            }
            if(filter != nullptr) {
                env->DeleteLocalRef(filter);
            }

            if(env->ExceptionCheck()) {
                return JNI_FALSE;
            }
            batch.push_back(std::move(item));
        }

        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        return native->manager.addVectors(index_id, batch) ? JNI_TRUE : JNI_FALSE;
    } catch(const std::invalid_argument& e) {
        throwIllegalArgument(env, e.what());
        return JNI_FALSE;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_endee_ndd_EndeeNative_searchJson(JNIEnv* env,
                                         jclass,
                                         jlong handle,
                                         jstring index_name,
                                         jfloatArray query,
                                         jint k,
                                         jstring filter_json,
                                         jboolean include_vectors,
                                         jint ef) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return nullptr;
        }

        nlohmann::json filter = nlohmann::json::array();
        const std::string filter_value = toString(env, filter_json, "filterJson", true);
        if(!filter_value.empty()) {
            filter = nlohmann::json::parse(filter_value);
        }

        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        auto results = native->manager.searchKNN(index_id,
                                                 toFloatVector(env, query, "query"),
                                                 static_cast<size_t>(k),
                                                 filter,
                                                 {},
                                                 include_vectors == JNI_TRUE,
                                                 ef > 0 ? static_cast<size_t>(ef) : 0);
        if(!results.has_value()) {
            throw std::runtime_error("Search failed");
        }

        nlohmann::json payload = nlohmann::json::array();
        for(const auto& result : *results) {
            payload.push_back(resultToJson(result));
        }
        return toJString(env, payload);
    } catch(const nlohmann::json::exception& e) {
        throwIllegalArgument(env, e.what());
        return nullptr;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return nullptr;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_endee_ndd_EndeeNative_getVectorJson(JNIEnv* env,
                                            jclass,
                                            jlong handle,
                                            jstring index_name,
                                            jstring id) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return nullptr;
        }

        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        auto vector = native->manager.getVector(index_id, toString(env, id, "id"));
        if(!vector.has_value()) {
            return toJString(env, nullptr);
        }

        nlohmann::json payload = {
                {"id", vector->id},
                {"meta", vector->meta},
                {"filter", vector->filter},
                {"norm", vector->norm},
                {"vector", vector->vector}};
        if(!vector->sparse_ids.empty()) {
            payload["sparse_indices"] = vector->sparse_ids;
            payload["sparse_values"] = vector->sparse_values;
        }
        return toJString(env, payload);
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return nullptr;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_endee_ndd_EndeeNative_deleteVector(JNIEnv* env,
                                           jclass,
                                           jlong handle,
                                           jstring index_name,
                                           jstring id) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return JNI_FALSE;
        }
        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        return native->manager.deleteVector(index_id, toString(env, id, "id")) ? JNI_TRUE
                                                                               : JNI_FALSE;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_io_endee_ndd_EndeeNative_updateFilters(JNIEnv* env,
                                            jclass,
                                            jlong handle,
                                            jstring index_name,
                                            jobjectArray ids,
                                            jobjectArray filters) {
    try {
        NativeHandle* native = requireHandle(env, handle);
        if(native == nullptr) {
            return 0;
        }
        if(ids == nullptr || filters == nullptr) {
            throw std::invalid_argument("ids and filters must not be null");
        }
        const jsize count = env->GetArrayLength(ids);
        if(env->GetArrayLength(filters) != count) {
            throw std::invalid_argument("ids and filters must have the same length");
        }

        std::vector<std::pair<std::string, std::string>> updates;
        updates.reserve(static_cast<size_t>(count));
        for(jsize i = 0; i < count; ++i) {
            auto id = static_cast<jstring>(env->GetObjectArrayElement(ids, i));
            auto filter = static_cast<jstring>(env->GetObjectArrayElement(filters, i));
            updates.emplace_back(toString(env, id, "ids[]"), toString(env, filter, "filters[]"));
            env->DeleteLocalRef(id);
            env->DeleteLocalRef(filter);
            if(env->ExceptionCheck()) {
                return 0;
            }
        }

        const std::string index_id = fullIndexId(toString(env, index_name, "indexName"));
        return static_cast<jint>(native->manager.updateFilters(index_id, updates));
    } catch(const std::invalid_argument& e) {
        throwIllegalArgument(env, e.what());
        return 0;
    } catch(const std::exception& e) {
        throwIllegalState(env, e.what());
        return 0;
    }
}
