package cn.campus.timetable;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebSettings;
import android.webkit.WebResourceRequest;
import android.webkit.ValueCallback;
import android.webkit.JavascriptInterface;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;

public class MainActivity extends Activity {
    private WebView web;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(237,246,253));
        getWindow().setNavigationBarColor(Color.WHITE);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        web = new WebView(this);
        web.setBackgroundColor(Color.rgb(242,248,252));
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setCacheMode(WebSettings.LOAD_NO_CACHE);
        s.setLoadsImagesAutomatically(true);
        s.setAllowFileAccess(true);
        s.setAllowContentAccess(false);
        s.setAllowFileAccessFromFileURLs(false);
        s.setAllowUniversalAccessFromFileURLs(false);
        s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        s.setBuiltInZoomControls(false);
        web.setWebViewClient(new WebViewClient(){
            @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest req){return !req.getUrl().toString().startsWith("file:///android_asset/");}
            @Override public boolean shouldOverrideUrlLoading(WebView view, String url){return !url.startsWith("file:///android_asset/");}
        });
        web.addJavascriptInterface(new WidgetBridge(),"WidgetBridge");
        setContentView(web);
        web.loadUrl("file:///android_asset/index.html");
    }
    public final class WidgetBridge {
        @JavascriptInterface public boolean syncSchedule(String json) {
            boolean saved=WidgetStore.save(getApplicationContext(),json);
            if(saved)ScheduleWidgetProvider.updateAll(getApplicationContext());
            return saved;
        }
        @JavascriptInterface public String requestPinWidget() {
            final AppWidgetManager manager=AppWidgetManager.getInstance(MainActivity.this);
            if(!manager.isRequestPinAppWidgetSupported())return "unsupported";
            runOnUiThread(new Runnable(){@Override public void run(){
                manager.requestPinAppWidget(new ComponentName(MainActivity.this,ScheduleWidgetProvider.class),null,null);
            }});
            return "requested";
        }
    }
    @Override public void onBackPressed(){
        web.evaluateJavascript("(function(){return window.closeSheet ? window.closeSheet() : false;})()", new ValueCallback<String>() {
            @Override public void onReceiveValue(String value) { if(!"true".equals(value)) finish(); }
        });
    }
    @Override protected void onPause(){super.onPause();web.onPause();}
    @Override protected void onResume(){super.onResume();if(web!=null)web.onResume();}
    @Override protected void onDestroy(){if(web!=null)web.destroy();super.onDestroy();}
}
