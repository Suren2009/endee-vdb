package io.endee.ndd.app;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

import io.endee.ndd.EndeeNative;
import io.endee.ndd.app.R;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;

public class MainActivity extends Activity {

    private static final String TAG = "EndeeDemoApp";

    private TextView logTextView;
    private ScrollView logScrollView;
    private View statusIndicator;
    private TextView statusText;
    private Button btnRunDemo;
    private Button btnClearLogs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        logTextView = findViewById(R.id.log_text_view);
        logScrollView = findViewById(R.id.log_scroll_view);
        statusIndicator = findViewById(R.id.status_indicator);
        statusText = findViewById(R.id.status_text);
        btnRunDemo = findViewById(R.id.btn_run_demo);
        btnClearLogs = findViewById(R.id.btn_clear_logs);

        btnRunDemo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runDemo();
            }
        });

        btnClearLogs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                logTextView.setText("");
                log("[INFO] Console cleared.\n");
            }
        });
    }

    private void runDemo() {
        btnRunDemo.setEnabled(false);
        statusText.setText("Status: Running Demo...");
        statusText.setTextColor(0xFFE2E8F0); // Reset color
        
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    log("[INFO] Starting Endee Vector DB execution...");
                    
                    File dataDir = new File(getFilesDir(), "endee");
                    if (!dataDir.exists()) {
                        dataDir.mkdirs();
                    }
                    String path = dataDir.getAbsolutePath();
                    log("[INFO] DB Data Directory: " + path);
                    
                    log("[INFO] Initializing EndeeNative instance...");
                    try (EndeeNative db = new EndeeNative(path)) {
                        String indexName = "demo";
                        int dimension = 4;

                        // Delete existing index to run from clean slate
                        log("[INFO] Checking if old index exists and deleting...");
                        db.deleteIndex(indexName);

                        // Create index
                        log("[INFO] Creating index '" + indexName + "'...");
                        boolean created = db.createIndex(
                            indexName,
                            dimension,
                            "cosine",
                            "float32",
                            10000,
                            16,
                            200,
                            0,
                            ""
                        );
                        if (!created) {
                            throw new RuntimeException("Failed to create index");
                        }
                        log("[SUCCESS] Created index: " + indexName + " (dim=" + dimension + ", cosine, float32)");

                        // Insert vectors
                        log("[INFO] Inserting vectors doc-1 and doc-2...");
                        String[] ids = new String[]{"doc-1", "doc-2"};
                        float[][] vectors = new float[][]{
                            {1.0f, 0.0f, 0.0f, 0.0f},
                            {0.9f, 0.1f, 0.0f, 0.0f}
                        };
                        byte[][] metadata = new byte[][]{new byte[0], new byte[0]};
                        String[] filters = new String[]{null, null};
                        float[] norms = new float[]{1.0f, 1.0f};

                        boolean added = db.addVectors(indexName, ids, vectors, metadata, filters, norms);
                        if (!added) {
                            throw new RuntimeException("Failed to add vectors");
                        }
                        log("[SUCCESS] Successfully added 2 vectors.");

                        // k-NN search
                        log("[INFO] Executing k-NN search with query: [1.0, 0.0, 0.0, 0.0]...");
                        float[] query = new float[]{1.0f, 0.0f, 0.0f, 0.0f};
                        String resultsJson = db.searchJson(
                            indexName,
                            query,
                            2,
                            "",
                            false,
                            100
                        );
                        log("[SUCCESS] Search result JSON:\n" + resultsJson);

                        // Parse search result
                        JSONArray hits = new JSONArray(resultsJson);
                        log("[INFO] Search results parsed successfully (" + hits.length() + " hits):");
                        for (int i = 0; i < hits.length(); i++) {
                            JSONObject hit = hits.getJSONObject(i);
                            String id = hit.getString("id");
                            double score = hit.getDouble("score");
                            log("  -> Hit #" + (i + 1) + ": Doc ID = '" + id + "', Similarity = " + String.format("%.6f", score));
                        }

                        // Index info
                        log("[INFO] Querying index info for '" + indexName + "'...");
                        String infoJson = db.getIndexInfoJson(indexName);
                        log("[SUCCESS] Index info: " + infoJson);

                        // List indexes
                        log("[INFO] Querying active database indexes...");
                        String listJson = db.listIndexesJson();
                        log("[SUCCESS] Active Indexes list: " + listJson);
                    }
                    
                    log("[FINISHED] Demo execution successfully completed!\n");
                    updateStatus("Status: Ready (Success)", true);
                } catch (final Exception e) {
                    log("[ERROR] Execution failed: " + e.getMessage() + "\n");
                    Log.e(TAG, "Error running vector DB demo", e);
                    updateStatus("Status: Ready (Error)", false);
                } finally {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            btnRunDemo.setEnabled(true);
                        }
                    });
                }
            }
        }).start();
    }

    private void log(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                logTextView.append(message + "\n");
                logScrollView.post(new Runnable() {
                    @Override
                    public void run() {
                        logScrollView.fullScroll(View.FOCUS_DOWN);
                    }
                });
            }
        });
    }

    private void updateStatus(final String text, final boolean isSuccess) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                statusText.setText(text);
                if (isSuccess) {
                    statusText.setTextColor(0xFF4ADE80); // Green
                } else {
                    statusText.setTextColor(0xFFF87171); // Red
                }
            }
        });
    }
}
