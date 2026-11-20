package com.nfsmw.diagnostics;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.view.Gravity;

public final class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView status = new TextView(this);
        status.setGravity(Gravity.CENTER);
        status.setTextSize(18);
        status.setPadding(32, 32, 32, 32);
        try {
            System.loadLibrary("nfsmw_native_diagnostics");
            status.setText("DIAGNÓSTICO ARM64\n\nBiblioteca nativa cargada.\nRevisa Logcat con la etiqueta NFSMW-Native-Diagnostics para el estado de Vulkan.");
        } catch (UnsatisfiedLinkError error) {
            status.setText("ERROR AL CARGAR LA BIBLIOTECA NATIVA\n\n" + error.getMessage());
        }
        setContentView(status);
    }
}
