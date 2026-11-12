package cn.campus.timetable;

import java.time.LocalDate;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/** Pure scheduling logic shared by the native widget and its JVM tests. */
public final class WidgetSchedule {
    public static final class Course {
        public String title, room;
        public int day, start, end;
        public int[] weeks;
        public Course(String title, String room, int day, int start, int end, int[] weeks) {
            this.title=title; this.room=room; this.day=day; this.start=start; this.end=end; this.weeks=weeks;
        }
    }
    public static int week(LocalDate date, LocalDate anchor) {
        return (int)Math.floorDiv(ChronoUnit.DAYS.between(anchor,date),7L)+1;
    }
    public static List<Course> forDay(List<Course> rules,LocalDate date,LocalDate anchor) {
        List<Course> found=new ArrayList<Course>();
        int w=week(date,anchor),day=date.getDayOfWeek().getValue();
        for(Course c:rules)if(c.day==day)for(int x:c.weeks)if(x==w){found.add(c);break;}
        Collections.sort(found,new Comparator<Course>() {public int compare(Course a,Course b){return a.start-b.start;}});
        return found;
    }
    public static int minutes(String value) {
        if(value==null||!value.matches("\\d{2}:\\d{2}"))return -1;
        int h=Integer.parseInt(value.substring(0,2)),m=Integer.parseInt(value.substring(3));
        return h<24&&m<60?h*60+m:-1;
    }
    public static int firstRemaining(List<Course> courses,String[][] times,int now) {
        for(int i=0;i<courses.size();i++){
            int end=minutes(times[courses.get(i).end-1][1]);
            if(end<0||end>now)return i;
        }
        return courses.size();
    }
    public static List<Course> visible(List<Course> courses,String[][] times,int now,int capacity) {
        int start=firstRemaining(courses,times,now);
        if(courses.size()<=capacity)start=0;
        else if(start==courses.size())start=Math.max(0,courses.size()-capacity);
        return new ArrayList<Course>(courses.subList(start,Math.min(courses.size(),start+Math.max(1,capacity))));
    }
}
