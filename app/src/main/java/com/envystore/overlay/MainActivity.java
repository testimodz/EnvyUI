package com.envystore.overlay;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Toast;

import java.io.IOException;

public class MainActivity extends Activity {

    private static final int REQ_OVERLAY = 1001;

    // Langsung exec su -c id → trigger Magisk/KernelSU popup
    private void requestRootAccess(final Runnable onGranted, final Runnable onDenied) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                boolean granted = false;
                try {
                    Process proc = Runtime.getRuntime().exec(new String[]{"su", "-c", "id"});
                    int exitCode = proc.waitFor();
                    granted = (exitCode == 0);
                } catch (IOException | InterruptedException e) {
                    granted = false;
                }
                final boolean result = granted;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (result) {
                            onGranted.run();
                        } else {
                            onDenied.run();
                        }
                    }
                });
            }
        }).start();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Step 1: Load native library
        try {
            System.loadLibrary("MEOW");
        } catch (UnsatisfiedLinkError e) {
            new AlertDialog.Builder(this)
                .setTitle("Envy - Error")
                .setMessage("Gagal load native library:\n" + e.getMessage())
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        finish();
                    }
                })
                .setCancelable(false)
                .show();
            return;
        }

        // Step 2: Langsung exec su — Magisk/KernelSU popup akan muncul
        Toast.makeText(this, "Meminta akses root...", Toast.LENGTH_SHORT).show();
        requestRootAccess(
            new Runnable() {
                @Override
                public void run() {
                    requestOverlayIfNeeded();
                }
            },
            new Runnable() {
                @Override
                public void run() {
                    new AlertDialog.Builder(MainActivity.this)
                        .setTitle("Envy - Root Ditolak")
                        .setMessage("Akses root ditolak atau HP tidak di-root.\n\nGunakan Magisk / KernelSU, lalu izinkan app ini dan coba lagi.")
                        .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        })
                        .setCancelable(false)
                        .show();
                }
            }
        );
    }

    private void requestOverlayIfNeeded() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            new AlertDialog.Builder(this)
                .setTitle("Envy - Izin Overlay")
                .setMessage("Izin \"Draw over other apps\" diperlukan agar mod menu bisa tampil di atas game.")
                .setPositiveButton("Izinkan", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Intent intent = new Intent(
                            Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                            Uri.parse("package:" + getPackageName())
                        );
                        startActivityForResult(intent, REQ_OVERLAY);
                    }
                })
                .setNegativeButton("Batal", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Toast.makeText(MainActivity.this, "Izin overlay diperlukan!", Toast.LENGTH_SHORT).show();
                        finish();
                    }
                })
                .setCancelable(false)
                .show();
        } else {
            Toast.makeText(this, "Envy aktif — buka Toram Online", Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQ_OVERLAY) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Settings.canDrawOverlays(this)) {
                Toast.makeText(this, "Envy aktif — buka Toram Online", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "Izin overlay ditolak!", Toast.LENGTH_SHORT).show();
            }
            finish();
        }
    }
}
