package cn.campus.timetable;

import android.content.Context;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;

public final class WidgetStore {
    public static final String PREFS="desktop_widget";
    public static final class Data {
        public LocalDate anchor;
        public int last;
        public boolean weekendConfirmed;
        public String character;
        public String[][] times=new String[12][2];
        public List<WidgetSchedule.Course> courses=new ArrayList<WidgetSchedule.Course>();
    }
    public static Data parse(String text) throws Exception {
        JSONObject root=new JSONObject(text),p=root.getJSONObject("prefs"),catalog=root.getJSONObject("catalog");
        Data data=new Data(); data.anchor=LocalDate.parse(p.getString("anchor"));
        if(data.anchor.getDayOfWeek().getValue()!=1)throw new IllegalArgumentException("Monday required");
        data.last=p.getInt("last");if(data.last<1||data.last>30)throw new IllegalArgumentException("Invalid term");
        data.character=p.optString("character","cheer");
        if(!data.character.matches("happy|cheer|sleep|friends|guitar"))data.character="cheer";
        data.weekendConfirmed=p.optBoolean("weekendConfirmed",false);
        JSONArray times=root.getJSONArray("times"),rules=root.getJSONArray("rules");
        if(times.length()!=12||rules.length()>1000)throw new IllegalArgumentException("Invalid data size");
        for(int i=0;i<12;i++)for(int j=0;j<2;j++){
            String t=times.getJSONArray(i).getString(j);
            if(!t.isEmpty()&&WidgetSchedule.minutes(t)<0)throw new IllegalArgumentException("Invalid time");
            data.times[i][j]=t;
        }
        for(int i=0;i<rules.length();i++){
            JSONObject r=rules.getJSONObject(i);
            int day=r.getInt("day"),start=r.getInt("start"),end=r.getInt("end");
            if(day<1||day>7||start<1||end>12||start>end)throw new IllegalArgumentException("Invalid period");
            JSONArray ws=r.getJSONArray("weeks");int[] weeks=new int[ws.length()];
            for(int j=0;j<weeks.length;j++){weeks[j]=ws.getInt(j);if(weeks[j]<1||weeks[j]>30)throw new IllegalArgumentException("Invalid week");}
            String title=catalog.getJSONArray(r.getString("course")).getString(1);
            data.courses.add(new WidgetSchedule.Course(title,r.optString("room","地点待确认"),day,start,end,weeks));
        }
        return data;
    }
    public static boolean save(Context context,String json) {
        try{
            if(json==null||json.length()>500000)return false;
            parse(json);
            return context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().putString("schedule",json).commit();
        }catch(Exception e){return false;}
    }
    public static Data load(Context context) throws Exception {
        String saved=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString("schedule",null);
        if(saved!=null)try{return parse(saved);}catch(Exception ignored){}
        try(InputStream in=context.getAssets().open("widget-default.json")){
            ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buf=new byte[8192];int n;
            while((n=in.read(buf))!=-1)out.write(buf,0,n);
            return parse(out.toString("UTF-8"));
        }
    }
}
