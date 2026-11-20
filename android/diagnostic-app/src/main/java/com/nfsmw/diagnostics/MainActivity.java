package com.nfsmw.diagnostics;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.LinearLayout;
import android.widget.TextView;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    private TextView status;
    private boolean probeStarted;

    private native String nativeRunDiagnostics(Surface surface);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        status = new TextView(this);
        status.setGravity(Gravity.CENTER);
        status.setTextSize(16);
        status.setPadding(24, 24, 24, 24);
        status.setText("DIAGNÓSTICO SDL3 + VULKAN\n\nEsperando la superficie Android...");

        SurfaceView surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.addView(status, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        layout.addView(surfaceView, new LinearLayout.LayoutParams(
                LinearLayout.MATCH_PARENT, 240));
        setContentView(layout);

        try {
            System.loadLibrary("nfsmw_native_diagnostics");
        } catch (UnsatisfiedLinkError error) {
            status.setText("ERROR AL CARGAR LA BIBLIOTECA NATIVA\n\n" + error.getMessage());
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (probeStarted) return;
        probeStarted = true;
        try {
            status.setText("DIAGNÓSTICO SDL3 + VULKAN\n\n" +
                    nativeRunDiagnostics(holder.getSurface()));
        } catch (UnsatisfiedLinkError error) {
            status.setText("ERROR EN LA PRUEBA NATIVA\n\n" + error.getMessage());
        }
    }

    @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) { }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        probeStarted = false;
    }
}
