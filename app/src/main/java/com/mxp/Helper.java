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
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.KeyEvent;
import android.widget.Toast;

import java.io.DataOutputStream;
import java.io.IOException;

public class Helper {

    // ── State fields ──────────────────────────────────────────────
    public static boolean isCountingDown     = false;
    public static boolean isSecureActive     = false;
    public static boolean isVpnWatcherActive = false;

    // Bug 1 fix: lazy-init Handler so it's never created before main Looper is ready
    public static Handler      keyRepeatHandler     = null;
    public static EditText     keyboardEditText     = null;
    public static TextWatcher  secureWatcherHandler = null;
    public static ConnectivityManager.NetworkCallback vpnWatcherHandler = null;

    private static Activity  sActivity = null;
    private static ViewGroup sRootView = null;

    private static Handler getMainHandler() {
        if (keyRepeatHandler == null)
            keyRepeatHandler = new Handler(Looper.getMainLooper());
        return keyRepeatHandler;
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
            // Bug 2 fix: use `this` instead of secureWatcherHandler to remove/re-add self
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
            if (sActivity == null) return;
            ConnectivityManager cm = (ConnectivityManager)
                sActivity.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return;
            NetworkCapabilities caps = cm.getNetworkCapabilities(network);
            if (caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_VPN))
                isVpnWatcherActive = true;
        }
        @Override
        public void onLost(Network network) { isVpnWatcherActive = false; }
    }

    // ── init — called from JNI after LoadDex ──────────────────────
    public static void init(Activity activity) {
        sActivity = activity;
        sRootView = (ViewGroup) activity.getWindow().getDecorView().getRootView();
    }

    // ── requestRoot — called from JNI after init ──────────────────
    // Bug 7 fix: properly close streams and process after use
    public static void requestRoot(final Activity activity) {
        new Thread(() -> {
            Process p = null;
            DataOutputStream os = null;
            try {
                p = Runtime.getRuntime().exec("su");
                os = new DataOutputStream(p.getOutputStream());
                os.writeBytes("id\n");
                os.writeBytes("exit\n");
                os.flush();
                int exit = p.waitFor();
                if (exit != 0 && activity != null) {
                    final Activity act = activity;
                    act.runOnUiThread(() ->
                        Toast.makeText(act,
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

    // ── putisCountingDown — JNI setter ────────────────────────────
    public static void putisCountingDown(boolean val) {
        isCountingDown = val;
    }

    // ── mstartCountdownAndExit ────────────────────────────────────
    // Bug 4 fix: create Handler fresh on main looper inside method
    public static void mstartCountdownAndExit(final int seconds) {
        if (sActivity == null) return;
        isCountingDown = true;
        final Activity act = sActivity;
        act.runOnUiThread(() -> {
            Toast.makeText(act, "Exiting in " + seconds + "s...",
                Toast.LENGTH_LONG).show();
            new Handler(Looper.getMainLooper()).postDelayed(() -> {
                isCountingDown = false;
                act.finish();
                android.os.Process.killProcess(android.os.Process.myPid());
            }, seconds * 1000L);
        });
    }

    // ── Keyboard ──────────────────────────────────────────────────
    public static void showSoftKeyboard(boolean show) {
        if (sActivity == null) return;
        final Activity act = sActivity;
        act.runOnUiThread(() -> {
            if (show) {
                if (keyboardEditText == null) {
                    keyboardEditText = new EditText(act);
                    keyboardEditText.setLayoutParams(
                        new ViewGroup.LayoutParams(1, 1));
                    keyboardEditText.setAlpha(0f);
                    sRootView.addView(keyboardEditText);
                    secureWatcherHandler = new InputWatcher();
                    keyboardEditText.addTextChangedListener(secureWatcherHandler);
                    keyboardEditText.setOnKeyListener(new KeyHandler());
                }
                keyboardEditText.requestFocus();
                InputMethodManager imm = (InputMethodManager)
                    act.getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null)
                    imm.showSoftInput(keyboardEditText,
                        InputMethodManager.SHOW_FORCED);
            } else {
                if (keyboardEditText != null) {
                    InputMethodManager imm = (InputMethodManager)
                        act.getSystemService(Context.INPUT_METHOD_SERVICE);
                    if (imm != null)
                        imm.hideSoftInputFromWindow(
                            keyboardEditText.getWindowToken(), 0);
                }
            }
        });
    }

    // ── Screen security ───────────────────────────────────────────
    // Bug 3 fix: properly sync isSecureActive with actual flag state
    public static void hideScreen(boolean hide) {
        if (sActivity == null) return;
        final Activity act = sActivity;
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
    // Bug 5 fix: registerNetworkCallback must run on main thread
    public static void registerVpnWatcher() {
        if (sActivity == null || isVpnWatcherActive) return;
        final Activity act = sActivity;
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager)
                act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return;
            vpnWatcherHandler = new VpnCallback();
            android.net.NetworkRequest req = new android.net.NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
                .build();
            cm.registerNetworkCallback(req, vpnWatcherHandler);
            isVpnWatcherActive = true;
        });
    }

    public static void unregisterVpnWatcher() {
        if (sActivity == null || !isVpnWatcherActive || vpnWatcherHandler == null) return;
        final Activity act = sActivity;
        act.runOnUiThread(() -> {
            ConnectivityManager cm = (ConnectivityManager)
                act.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm != null) cm.unregisterNetworkCallback(vpnWatcherHandler);
            vpnWatcherHandler = null;
            isVpnWatcherActive = false;
        });
    }

    // ── IsVpnActive (static check) ────────────────────────────────
    // Bug 6 fix: fallback for SDK < M using getAllNetworks / getAllNetworkInfo
    public static boolean IsVpnActive() {
        if (sActivity == null) return false;
        ConnectivityManager cm = (ConnectivityManager)
            sActivity.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) return false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Network active = cm.getActiveNetwork();
            if (active == null) return false;
            NetworkCapabilities caps = cm.getNetworkCapabilities(active);
            return caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
        } else {
            // fallback: scan all NetworkInfo for TYPE_VPN
            NetworkInfo[] nets = cm.getAllNetworkInfo();
            if (nets == null) return false;
            for (NetworkInfo ni : nets) {
                if (ni.getType() == ConnectivityManager.TYPE_VPN && ni.isConnected())
                    return true;
            }
            return false;
        }
    }
}
