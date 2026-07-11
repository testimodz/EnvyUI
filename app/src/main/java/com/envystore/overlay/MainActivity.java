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

import java.io.File;
import java.io.IOException;

public class MainActivity extends Activity {

    private static final int REQ_OVERLAY = 1001;
    private boolean libLoaded = false;

    // Cek apakah binary su tersedia di path umum
    private boolean isSuBinaryPresent() {
        String[] paths = {
            "/su/bin/su",
            "/system/bin/su",
            "/system/xbin/su",
            "/sbin/su",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/data/local/su",
            "/system/sd/xbin/su",
            "/system/bin/failsafe/su",
            "/dev/com.koushikdutta.superuser.daemon/"
        };
        for (String path : paths) {
            if (new File(path).exists()) return true;
        }
        return false;
    }

    // Coba jalankan 'su -c id' untuk trigger popup root request (Magisk/KernelSU)
    private void requestRootAccess(final Runnable onGranted, final Runnable onDenied) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Process proc = Runtime.getRuntime().exec(new String[]{"su", "-c", "id"});
                    int exitCode = proc.waitFor();
                    final boolean granted = (exitCode == 0);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (granted) {
                                onGranted.run();
                            } else {
                                onDenied.run();
                            }
                        }
                    });
                } catch (IOException | InterruptedException e) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            onDenied.run();
                        }
                    });
                }
            }
        }).start();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Step 1: Load native library
        try {
            System.loadLibrary("MEOW");
            libLoaded = true;
        } catch (UnsatisfiedLinkError e) {
            new AlertDialog.Builder(this)
                .setTitle("Envy - Error")
                .setMessage("Gagal load native library:\n" + e.getMessage() + "\n\nPastikan build sukses dan ABI arm64-v8a sesuai.")
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

        // Step 2: Cek su binary dulu, lalu request root
        if (!isSuBinaryPresent()) {
            new AlertDialog.Builder(this)
                .setTitle("Envy - Root Required")
                .setMessage("HP kamu tidak terdeteksi root.\n\nMod ini butuh akses root (Magisk / KernelSU) untuk patch memory game.\nInstall Magisk terlebih dahulu.")
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

        // Step 3: Trigger root popup via su exec (Magisk/KernelSU akan muncul)
        Toast.makeText(this, "Meminta akses root...", Toast.LENGTH_SHORT).show();
        requestRootAccess(
            new Runnable() {
                @Override
                public void run() {
                    // Root granted — lanjut ke overlay
                    requestOverlayIfNeeded();
                }
            },
            new Runnable() {
                @Override
                public void run() {
                    // Root denied
                    new AlertDialog.Builder(MainActivity.this)
                        .setTitle("Envy - Root Ditolak")
                        .setMessage("Akses root ditolak.\n\nBuka Magisk / KernelSU dan izinkan aplikasi ini, lalu coba lagi.")
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
