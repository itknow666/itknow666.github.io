package cn.campus.timetable;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.widget.RemoteViews;
import cn.campus.blueschedule.R;
import java.io.InputStream;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.List;

public final class ScheduleWidgetProvider extends AppWidgetProvider {
    private static final String REFRESH="cn.campus.blueschedule.REFRESH_WIDGET";
    private static final String MIDNIGHT="cn.campus.blueschedule.WIDGET_MIDNIGHT";
    private static final String[] DAYS={"","一","二","三","四","五","六","日"};
    private static Bitmap cachedCharacter;
    private static String cachedName;

    @Override public void onUpdate(Context context,AppWidgetManager manager,int[] ids){updateAll(context);}
    @Override public void onAppWidgetOptionsChanged(Context context,AppWidgetManager manager,int id,Bundle options){render(context,manager,id);}
    @Override public void onDisabled(Context context){cancelMidnight(context);}
    @Override public void onReceive(Context context,Intent intent){
        super.onReceive(context,intent);
        String action=intent.getAction();
        if(REFRESH.equals(action)||MIDNIGHT.equals(action)||Intent.ACTION_BOOT_COMPLETED.equals(action)||Intent.ACTION_MY_PACKAGE_REPLACED.equals(action)||Intent.ACTION_TIME_CHANGED.equals(action)||Intent.ACTION_TIMEZONE_CHANGED.equals(action)||Intent.ACTION_DATE_CHANGED.equals(action))updateAll(context);
    }
    public static synchronized void updateAll(Context context){
        AppWidgetManager manager=AppWidgetManager.getInstance(context);
        int[] ids=manager.getAppWidgetIds(new ComponentName(context,ScheduleWidgetProvider.class));
        for(int id:ids)render(context,manager,id);
        if(ids.length>0)scheduleMidnight(context);else cancelMidnight(context);
    }
    private static PendingIntent broadcast(Context context,String action,int code){
        Intent intent=new Intent(context,ScheduleWidgetProvider.class).setAction(action);
        return PendingIntent.getBroadcast(context,code,intent,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
    }
    private static void scheduleMidnight(Context context){
        AlarmManager alarm=(AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        long next=LocalDate.now().plusDays(1).atStartOfDay(ZoneId.systemDefault()).toInstant().toEpochMilli();
        if(alarm!=null)alarm.setWindow(AlarmManager.RTC,next,600000L,broadcast(context,MIDNIGHT,2));
    }
    private static void cancelMidnight(Context context){
        AlarmManager alarm=(AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        if(alarm!=null)alarm.cancel(broadcast(context,MIDNIGHT,2));
    }
    private static Bitmap character(Context context,String name) throws Exception {
        if(cachedCharacter!=null&&name.equals(cachedName))return cachedCharacter;
        BitmapFactory.Options options=new BitmapFactory.Options(); options.inSampleSize=4;
        Bitmap original;
        try(InputStream in=context.getAssets().open("characters/"+name+".jpg")){original=BitmapFactory.decodeStream(in,null,options);}
        if(original==null)throw new IllegalArgumentException("Image unavailable");
        int top="friends".equals(name)?960:"guitar".equals(name)?1190:"cheer".equals(name)?710:720;
        Bitmap result=Bitmap.createBitmap(112,100,Bitmap.Config.ARGB_8888);
        new Canvas(result).drawBitmap(original,new Rect(0,top*original.getHeight()/2400,original.getWidth(),(top+964)*original.getHeight()/2400),new Rect(0,0,112,100),new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG));
        original.recycle();cachedCharacter=result;cachedName=name;return result;
    }
    private static void render(Context context,AppWidgetManager manager,int id){
        RemoteViews views=new RemoteViews(context.getPackageName(),R.layout.widget_schedule);
        Intent launch=new Intent(context,MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent open=PendingIntent.getActivity(context,0,launch,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
        views.setOnClickPendingIntent(R.id.widget_root,open);
        views.setOnClickPendingIntent(R.id.widget_refresh,broadcast(context,REFRESH,1));
        views.removeAllViews(R.id.widget_rows);
        LocalDateTime now=LocalDateTime.now();LocalDate today=now.toLocalDate();
        try{
            WidgetStore.Data data=WidgetStore.load(context);
            int w=WidgetSchedule.week(today,data.anchor),day=today.getDayOfWeek().getValue();
            List<WidgetSchedule.Course> courses=WidgetSchedule.forDay(data.courses,today,data.anchor);
            String week=w>=1&&w<=data.last?"第"+w+"周":"学期外";
            views.setTextViewText(R.id.widget_date,today.getMonthValue()+"月"+today.getDayOfMonth()+"日 · 周"+DAYS[day]+" · "+week);
            try{views.setImageViewBitmap(R.id.widget_character,character(context,data.character));}catch(Exception ignored){}
            int minute=now.getHour()*60+now.getMinute(),remaining=WidgetSchedule.firstRemaining(courses,data.times,minute);
            String heading="小八 · 今日"+courses.size()+"门课";
            if(!courses.isEmpty()&&remaining==courses.size())heading="小八 · 今天的课已结束";
            views.setTextViewText(R.id.widget_title,heading);
            int height=manager.getAppWidgetOptions(id).getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_HEIGHT,170);
            int capacity=Math.max(1,Math.min(8,(height-84)/41));
            List<WidgetSchedule.Course> visible=WidgetSchedule.visible(courses,data.times,minute,capacity);
            if(courses.isEmpty()){
                String empty="今天没有已录入的课程\n小八陪你，慢慢来。";
                if(day>=6&&!data.weekendConfirmed)empty="周末课程待核对\n点击打开课表，补充周末安排。";
                if(w<1||w>data.last)empty="当前日期在学期范围外\n点击打开设置，核对学期起点。";
                RemoteViews row=new RemoteViews(context.getPackageName(),R.layout.widget_empty);
                row.setTextViewText(R.id.widget_empty_text,empty);views.addView(R.id.widget_rows,row);
            }else for(WidgetSchedule.Course c:visible){
                RemoteViews row=new RemoteViews(context.getPackageName(),R.layout.widget_course_row);
                String start=data.times[c.start-1][0],end=data.times[c.end-1][1];
                int starts=WidgetSchedule.minutes(start),ends=WidgetSchedule.minutes(end);
                String prefix=ends>=0&&ends<=minute?"已结束 · ":starts>=0&&starts<=minute?(ends<0?"结束待定 · ":"上课中 · "):"";
                row.setTextViewText(R.id.row_time,(start.isEmpty()?"时间待定":start)+"\n"+(end.isEmpty()?"结束待定":"— "+end));
                row.setTextViewText(R.id.row_title,prefix+c.title);
                row.setTextViewText(R.id.row_room,c.room.isEmpty()?"地点待确认":c.room);
                views.addView(R.id.widget_rows,row);
            }
            int hidden=courses.size()-visible.size();
            views.setTextViewText(R.id.widget_footer,(hidden>0?"另"+hidden+"门点开查看":"点课表打开")+" · 更新 "+now.format(DateTimeFormatter.ofPattern("HH:mm")));
        }catch(Exception e){
            views.setTextViewText(R.id.widget_title,"小八 · 点开同步课表");
            views.setTextViewText(R.id.widget_date,"打开应用后，桌面课表将自动更新");
            views.setTextViewText(R.id.widget_footer,"点击课表打开应用 · 右侧刷新");
        }
        manager.updateAppWidget(id,views);
    }
}
