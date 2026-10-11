package org.pricetracker;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import org.qtproject.qt.android.bindings.QtService;

public class PriceService extends QtService {
    private static final String CHANNEL = "background";
    private static final int NOTIFICATION_ID = 7000;

    public static void start(Context c) {
        Intent i = new Intent(c, PriceService.class);
        if (Build.VERSION.SDK_INT >= 26)
            c.startForegroundService(i);
        else
            c.startService(i);
    }

    public static void stop(Context c) {
        c.stopService(new Intent(c, PriceService.class));
    }

    @Override
    public void onCreate() {
        startForeground(NOTIFICATION_ID, buildNotification());
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        return START_STICKY;
    }

    private Notification buildNotification() {
        NotificationManager nm =
            (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        Notification.Builder b;
        if (Build.VERSION.SDK_INT >= 26) {
            nm.createNotificationChannel(new NotificationChannel(
                CHANNEL, "Background alerts", NotificationManager.IMPORTANCE_LOW));
            b = new Notification.Builder(this, CHANNEL);
        } else {
            b = new Notification.Builder(this);
        }
        Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
        PendingIntent pi = PendingIntent.getActivity(this, 0, launch, PendingIntent.FLAG_IMMUTABLE);
        b.setContentTitle("Gold & Dollar Tracker")
         .setContentText("Watching prices for your alerts")
         .setSmallIcon(getApplicationInfo().icon)
         .setContentIntent(pi)
         .setOngoing(true);
        return b.build();
    }
}
