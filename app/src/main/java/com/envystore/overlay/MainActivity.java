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

public class MainActivity extends Activity {

    static {
        System.loadLibrary("MEOW");
    }

    private static final int REQ_OVERLAY = 1001;

    private boolean isRooted() {
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!isRooted()) {
            new AlertDialog.Builder(this)
                .setTitle("Envy - Root Required")
                .setMessage("HP kamu tidak terdeteksi root.\nMod ini membutuhkan akses root untuk patch memory game.\nGunakan Magisk / KernelSU.")
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

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (!Settings.canDrawOverlays(this)) {
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
                return;
            }
        }

        Toast.makeText(this, "Envy aktif — buka Toram Online", Toast.LENGTH_SHORT).show();
        finish();
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
