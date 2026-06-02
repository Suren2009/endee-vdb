package io.endee.ndd;

/**
 * Thin Java wrapper around the Endee native vector database.
 *
 * <p>The native library name is {@code endee}, which maps to {@code libendee.so}
 * on Android. Methods that return records use JSON strings so callers can bind
 * them to their preferred JSON library or app models.
 */
public final class EndeeNative implements AutoCloseable {
    static {
        System.loadLibrary("endee");
    }

    private long handle;

    public EndeeNative(String dataDir) {
        this(dataDir, 10_000, 30, true);
    }

    public EndeeNative(
            String dataDir,
            int saveEveryNUpdates,
            int saveIntervalMinutes,
            boolean saveOnShutdown) {
        handle = createManager(dataDir, saveEveryNUpdates, saveIntervalMinutes, saveOnShutdown);
    }

    public synchronized boolean createIndex(
            String indexName,
            int dimension,
            String spaceType,
            String precision,
            int maxElements,
            int m,
            int efConstruction,
            int checksum,
            String sparseModel) {
        return createIndex(
                requireHandle(),
                indexName,
                dimension,
                spaceType,
                precision,
                maxElements,
                m,
                efConstruction,
                checksum,
                sparseModel);
    }

    public synchronized boolean deleteIndex(String indexName) {
        return deleteIndex(requireHandle(), indexName);
    }

    public synchronized String getIndexInfoJson(String indexName) {
        return getIndexInfoJson(requireHandle(), indexName);
    }

    public synchronized String listIndexesJson() {
        return listIndexesJson(requireHandle());
    }

    public synchronized boolean addVectors(
            String indexName,
            String[] ids,
            float[][] vectors,
            byte[][] metadata,
            String[] filters,
            float[] norms) {
        return addVectors(requireHandle(), indexName, ids, vectors, metadata, filters, norms);
    }

    public synchronized String searchJson(
            String indexName,
            float[] query,
            int k,
            String filterJson,
            boolean includeVectors,
            int ef) {
        return searchJson(requireHandle(), indexName, query, k, filterJson, includeVectors, ef);
    }

    public synchronized String getVectorJson(String indexName, String id) {
        return getVectorJson(requireHandle(), indexName, id);
    }

    public synchronized boolean deleteVector(String indexName, String id) {
        return deleteVector(requireHandle(), indexName, id);
    }

    public synchronized int updateFilters(String indexName, String[] ids, String[] filters) {
        return updateFilters(requireHandle(), indexName, ids, filters);
    }

    @Override
    public synchronized void close() {
        if (handle != 0) {
            destroyManager(handle);
            handle = 0;
        }
    }

    private long requireHandle() {
        if (handle == 0) {
            throw new IllegalStateException("EndeeNative is already closed");
        }
        return handle;
    }

    private static native long createManager(
            String dataDir,
            int saveEveryNUpdates,
            int saveIntervalMinutes,
            boolean saveOnShutdown);

    private static native void destroyManager(long handle);

    private static native boolean createIndex(
            long handle,
            String indexName,
            int dimension,
            String spaceType,
            String precision,
            int maxElements,
            int m,
            int efConstruction,
            int checksum,
            String sparseModel);

    private static native boolean deleteIndex(long handle, String indexName);

    private static native String getIndexInfoJson(long handle, String indexName);

    private static native String listIndexesJson(long handle);

    private static native boolean addVectors(
            long handle,
            String indexName,
            String[] ids,
            float[][] vectors,
            byte[][] metadata,
            String[] filters,
            float[] norms);

    private static native String searchJson(
            long handle,
            String indexName,
            float[] query,
            int k,
            String filterJson,
            boolean includeVectors,
            int ef);

    private static native String getVectorJson(long handle, String indexName, String id);

    private static native boolean deleteVector(long handle, String indexName, String id);

    private static native int updateFilters(
            long handle, String indexName, String[] ids, String[] filters);
}
