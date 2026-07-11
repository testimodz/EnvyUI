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

public class MainActivity extends Activity {

    static {
        System.loadLibrary("MEOW");
    }

    private static final int REQ_OVERLAY = 1001;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (!Settings.canDrawOverlays(this)) {
                new AlertDialog.Builder(this)
                    .setTitle("Envy")
                    .setMessage("Izin overlay diperlukan agar mod menu bisa tampil di atas game.")
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
                            Toast.makeText(MainActivity.this, "Izin diperlukan!", Toast.LENGTH_SHORT).show();
                            finish();
                        }
                    })
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
