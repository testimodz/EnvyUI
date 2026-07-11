package com.mxp;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.ref.WeakReference;

public class Helper {

    // ── State fields ──────────────────────────────────────────────
    public static boolean isCountingDown    = false;
    public static boolean isSecureActive    = false;
    
    // Flag status registrasi callback & status jaringan VPN
    public static boolean isVpnWatcherRegistered = false;
    public static boolean isVpnConnected         = false;

    private static Handler keyRepeatHandler = null;
    public static EditText keyboardEditText = null;
    public static TextWatcher secureWatcherHandler = null;
    public static ConnectivityManager.NetworkCallback vpnWatcherHandler = null;

    private static WeakReference<Activity> sActivityRef = null;
    private static ViewGroup sRootView = null;

    private static Handler getMainHandler() {
        if (keyRepeatHandler == null) {
            keyRepeatHandler = new Handler(Looper.getMainLooper());
        }
        return keyRepeatHandler;
    }

    private static Activity getActivity() {
        return sActivityRef != null ? sActivityRef.get() : null;
    }

    // ── Native callbacks (implemented in JNIStuff.h) ──────────────
    public static native void nativeAddChar(int codepoint);
    public static native void nativeKeyEvent(int keyCode);

    // ── TextWatcher ───────────────────────────────────────────────
    private static class InputWatcher implements TextWatcher {
        @Override public void beforeTextChanged(CharSequence s, int st, int c, int a) {}
        @Override public void onTextChanged(CharSequence s, int st, int b, int c) {}
        @Override
        public void afterTextChanged(Editable s) {
            if (keyboardEditText == null) return;
            String text = keyboardEditText.getText().toString();
            if (text.isEmpty()) return;
            
            for (int i = 0; i < text.length(); ) {
                int cp = text.codePointAt(i);
                nativeAddChar(cp);
                i += Character.charCount(cp);
            }
            keyboardEditText.removeTextChangedListener(this);
            keyboardEditText.setText("");
            keyboardEditText.addTextChangedListener(this);
        }
    }

    // ── KeyListener ───────────────────────────────────────────────
    private static class KeyHandler implements View.OnKeyListener {
        @Override
        public boolean onKey(View v, int keyCode, KeyEvent event) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                nativeKeyEvent(keyCode);
                return true;
            }
            return false;
        }
    }

    // ── VPN NetworkCallback ───────────────────────────────────────
    private static class VpnCallback extends ConnectivityManager.NetworkCallback {
        @Override
        public void onAvailable(Network network) {
            Activity act = getActivity();
            if (act == null) return;
            ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return;
            NetworkCapabilities caps = cm.getNetworkCapabilities(network);
            if (caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_VPN)) {
                isVpnConnected = true;
            }
        }
        @Override
        public void onLost(Network network) { 
            isVpnConnected = false; 
        }
    }

    // ── init — called from JNI after LoadDex ──────────────────────
    public static void init(Activity activity) {
        sActivityRef = new WeakReference<>(activity);
        sRootView = (ViewGroup) activity.getWindow().getDecorView().getRootView();
    }

    public static void cleanup() {
        unregisterVpnWatcher();
        if (sRootView != null && keyboardEditText != null) {
            sRootView.removeView(keyboardEditText);
            keyboardEditText = null;
        }
        sActivityRef = null;
        sRootView = null;
    }

    // ── requestRoot — Penulisan Thread & Lambda sudah dicek total ──
    public static void requestRoot(final Activity activity) {
        new Thread(() -> {
            Process p = null;
            DataOutputStream os = null;
            try {
                p = Runtime.getRuntime().exec("su");
                os = new DataOutputStream(p.getOutputStream());
                
                // Pembersihan syntax thread pendukung stream consumption
                final Process finalP = p;
                new Thread(() -> consumeStream(finalP.getInputStream())).start();
                new Thread(() -> consumeStream(finalP.getErrorStream())).start();

                os.writeBytes("id\n");
                os.writeBytes("exit\n");
                os.flush();
                
                int exit = p.waitFor();
                if (exit != 0 && activity != null) {
                    activity.runOnUiThread(() ->
                        Toast.makeText(activity,
                            "Root not granted. Some features may be limited.",
                            Toast.LENGTH_LONG).show()
                    );
                }
            } catch (Exception e) {
                if (activity != null) {
                    final String msg = e.getMessage();
                    activity.runOnUiThread(() ->
                        Toast.makeText(activity,
                            "Root unavailable: " + msg,
                            Toast.LENGTH_LONG).show()
                    );
                }
            } finally {
                if (os != null) { try { os.close(); } catch (IOException ignored) {} }
                if (p  != null) p.destroy();
            }
        }).start();
    }

    private static void consumeStream(java.io.InputStream is) {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
            while (reader.readLine() != null) {
                // Mengosongkan buffer stream
            }
        } catch (IOException ignored) {}
    }

    // ── putisCountingDown — JNI setter ────────────────────────────
    public static void putisCountingDown(boolean val) {
        isCountingDown = val;
    }

    // ── mstartCountdownAndExit ────────────────────────────────────
    public static void mstartCountdownAndExit(final int seconds) {
        Activity act = getActivity();
        if (act == null) return;
        isCountingDown = true;
        
        act.runOnUiThread(() -> {
            Toast.makeText(act, "Exiting in " + seconds + "s...", Toast.LENGTH_LONG).show();
            getMainHandler().postDelayed(() -> {
                isCountingDown = false;
                Activity currentAct = getActivity();
                if (currentAct != null) currentAct.finish();
                android.os.Process.killProcess(android.os.Process.myPid());
            }, seconds * 1000L);
        });
    }

    // ── Keyboard ──────────────────────────────────────────────────
    public static void showSoftKeyboard(boolean show) {
        Activity act = getActivity();
        if (act == null || sRootView == null) return;
        
        act.runOnUiThread(() -> {
            InputMethodManager imm = (InputMethodManager) act.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (show) {
                if (keyboardEditText == null) {
                    keyboardEditText = new EditText(act);
                    keyboardEditText.setLayoutParams(new ViewGroup.LayoutParams(1, 1));
                    keyboardEditText.setAlpha(0f);
                    keyboardEditText.setImeOptions(android.view.inputmethod.EditorInfo.IME_FLAG_NO_EXTRACT_UI);
                    sRootView.addView(keyboardEditText);
                    
                    secureWatcherHandler = new InputWatcher();
                    keyboardEditText.addTextChangedListener(secureWatcherHandler);
                    keyboardEditText.setOnKeyListener(new KeyHandler());
                }
                keyboardEditText.requestFocus();
                if (imm != null) {
                    imm.showSoftInput(keyboardEditText, InputMethodManager.SHOW_FORCED);
                }
            } else {
                if (keyboardEditText != null && imm != null) {
                    keyboardEditText.clearFocus();
                    imm.hideSoftInputFromWindow(keyboardEditText.getWindowToken(), 0);
                }
            }
        });
    }

    // ── Screen security ───────────────────────────────────────────
    public static void hideScreen(boolean hide) {
        Activity act = getActivity();
        if (act == null) return;
        
        act.runOnUiThread(() -> {
            if (hide) {
                act.getWindow().setFlags(
                    android.view.WindowManager.LayoutParams.FLAG_SECURE,
                    android.view.WindowManager.LayoutParams.FLAG_SECURE);
                isSecureActive = true;
            } else {
                act.getWindow().clearFlags(
                    android.view.WindowManager.LayoutParams.FLAG_SECURE);
                isSecureActive = false;
            }
        });
    }

    // ── VPN watcher ───────────────────────────────────────────────
    public static void registerVpnWatcher() {
        Activity act = getActivity();
        if (act == null || isVpnWatcherRegistered) return;
        
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return;
            
            vpnWatcherHandler = new VpnCallback();
            android.net.NetworkRequest req = new android.net.NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
                .build();
            cm.registerNetworkCallback(req, vpnWatcherHandler);
            isVpnWatcherRegistered = true;
        });
    }

    public static void unregisterVpnWatcher() {
        Activity act = getActivity();
        if (act == null || !isVpnWatcherRegistered || vpnWatcherHandler == null) return;
        
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm != null) {
                try {
                    cm.unregisterNetworkCallback(vpnWatcherHandler);
                } catch (IllegalArgumentException ignored) {}
            }
            vpnWatcherHandler = null;
            isVpnWatcherRegistered = false;
            isVpnConnected = false;
        });
    }

    // ── IsVpnActive (static check) ────────────────────────────────
    public static boolean IsVpnActive() {
        Activity act = getActivity();
        if (act == null) return false;
        ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) return false;
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Network active = cm.getActiveNetwork();
            if (active == null) return false;
            NetworkCapabilities caps = cm.getNetworkCapabilities(active);
            return caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
        } else {
            NetworkInfo[] nets = cm.getAllNetworkInfo();
            if (nets == null) return false;
            for (NetworkInfo ni : nets) {
                if (ni.getType() == ConnectivityManager.TYPE_VPN && ni.isConnected()) {
                    return true;
                }
            }
            return false;
        }
    }
}
