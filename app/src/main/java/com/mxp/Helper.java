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
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

public class Helper {

    // ── State fields ──────────────────────────────────────────────
    private static final AtomicBoolean isCountingDown = new AtomicBoolean(false);
    private static final AtomicBoolean isSecureActive = new AtomicBoolean(false);
    
    // Flag status registrasi callback & status jaringan VPN
    private static final AtomicBoolean isVpnWatcherRegistered = new AtomicBoolean(false);
    private static final AtomicBoolean isVpnConnected = new AtomicBoolean(false);

    private static Handler keyRepeatHandler = null;
    private static final AtomicReference<EditText> keyboardEditText = new AtomicReference<>(null);
    private static final AtomicReference<TextWatcher> secureWatcherHandler = new AtomicReference<>(null);
    private static final AtomicReference<ConnectivityManager.NetworkCallback> vpnWatcherHandler = new AtomicReference<>(null);

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
            EditText editText = keyboardEditText.get();
            if (editText == null) return;
            
            String text = editText.getText().toString();
            if (text.isEmpty()) return;
            
            for (int i = 0; i < text.length(); ) {
                int cp = text.codePointAt(i);
                nativeAddChar(cp);
                i += Character.charCount(cp);
            }
            editText.removeTextChangedListener(this);
            editText.setText("");
            editText.addTextChangedListener(this);
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
                isVpnConnected.set(true);
            }
        }
        @Override
        public void onLost(Network network) { 
            isVpnConnected.set(false); 
        }
    }

    // ── init — called from JNI after LoadDex ──────────────────────
    public static void init(Activity activity) {
        sActivityRef = new WeakReference<>(activity);
        sRootView = (ViewGroup) activity.getWindow().getDecorView().getRootView();
    }

    public static void cleanup() {
        unregisterVpnWatcher();
        if (sRootView != null) {
            EditText editText = keyboardEditText.get();
            if (editText != null) {
                sRootView.removeView(editText);
                keyboardEditText.set(null);
            }
        }
        sActivityRef = null;
        sRootView = null;
    }

    // ── requestRoot — Thread & Lambda handling verified ──
    public static void requestRoot(final Activity activity) {
        new Thread(() -> {
            Process p = null;
            DataOutputStream os = null;
            try {
                p = Runtime.getRuntime().exec("su");
                os = new DataOutputStream(p.getOutputStream());
                
                // Stream consumption in separate threads to prevent deadlock
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
                if (p != null) p.destroy();
            }
        }).start();
    }

    private static void consumeStream(java.io.InputStream is) {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
            while (reader.readLine() != null) {
                // Clear stream buffer
            }
        } catch (IOException ignored) {}
    }

    // ── putisCountingDown — JNI setter ────────────────────────────
    public static void putisCountingDown(boolean val) {
        isCountingDown.set(val);
    }

    // ── mstartCountdownAndExit ────────────────────────────────────
    public static void mstartCountdownAndExit(final int seconds) {
        Activity act = getActivity();
        if (act == null) return;
        isCountingDown.set(true);
        
        act.runOnUiThread(() -> {
            Toast.makeText(act, "Exiting in " + seconds + "s...", Toast.LENGTH_LONG).show();
            getMainHandler().postDelayed(() -> {
                isCountingDown.set(false);
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
                EditText editText = keyboardEditText.get();
                if (editText == null) {
                    editText = new EditText(act);
                    editText.setLayoutParams(new ViewGroup.LayoutParams(1, 1));
                    editText.setAlpha(0f);
                    editText.setImeOptions(android.view.inputmethod.EditorInfo.IME_FLAG_NO_EXTRACT_UI);
                    sRootView.addView(editText);
                    
                    InputWatcher watcher = new InputWatcher();
                    secureWatcherHandler.set(watcher);
                    editText.addTextChangedListener(watcher);
                    editText.setOnKeyListener(new KeyHandler());
                    
                    keyboardEditText.set(editText);
                }
                editText.requestFocus();
                if (imm != null) {
                    imm.showSoftInput(editText, InputMethodManager.SHOW_FORCED);
                }
            } else {
                EditText editText = keyboardEditText.get();
                if (editText != null && imm != null) {
                    editText.clearFocus();
                    imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
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
                isSecureActive.set(true);
            } else {
                act.getWindow().clearFlags(
                    android.view.WindowManager.LayoutParams.FLAG_SECURE);
                isSecureActive.set(false);
            }
        });
    }

    // ── VPN watcher ───────────────────────────────────────────────
    public static void registerVpnWatcher() {
        Activity act = getActivity();
        if (act == null || isVpnWatcherRegistered.get()) return;
        
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return;
            
            VpnCallback callback = new VpnCallback();
            vpnWatcherHandler.set(callback);
            android.net.NetworkRequest req = new android.net.NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
                .build();
            cm.registerNetworkCallback(req, callback);
            isVpnWatcherRegistered.set(true);
        });
    }

    public static void unregisterVpnWatcher() {
        Activity act = getActivity();
        if (act == null || !isVpnWatcherRegistered.get() || vpnWatcherHandler.get() == null) return;
        
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
            ConnectivityManager.NetworkCallback callback = vpnWatcherHandler.get();
            if (cm != null && callback != null) {
                try {
                    cm.unregisterNetworkCallback(callback);
                } catch (IllegalArgumentException ignored) {}
            }
            vpnWatcherHandler.set(null);
            isVpnWatcherRegistered.set(false);
            isVpnConnected.set(false);
        });
    }

    // ── isVpnActive (static check) ────────────────────────────────
    public static boolean isVpnActive() {
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
