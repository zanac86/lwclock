void Time_init()
{
#if USE_RTC == true
    uint8_t _tries = 5;
    while (--_tries && !rtc.begin())
    {
        Serial.println(F("Couldn't find RTC"));
        //while (1);
        delay(100);
    }
    if (_tries > 0)
    {
        Serial.println(F("RTC found"));
        if (rtc.lostPower())
        {
            Serial.println(F("RTC lost power, lets set the time!"));
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // following line sets the RTC to the date & time this sketch was compiled
            // This line sets the RTC with an explicit date & time, for example to set
            // January 21, 2014 at 3am you would call:
            // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
        }
    }
#endif
    timeSynch();
}

#if USE_RTC == true
void timeSynch()
{
    time_t gotTime;
    if (useRTC)
    {
        gotTime = getRtcTime();
        setTime(gotTime);
    }
    else
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            gotTime = getNtpTime();
            if (gotTime > 0)   //if got time from NTP set RTC
            {
                setTime(gotTime);
                time_t tn = now();
                rtc.adjust(DateTime(year(tn), month(tn), day(tn), hour(tn), minute(tn), second(tn)));
            }
            else
            {
                statusUpdateNtpTime = 0;
            }
        }
        else
        {
            gotTime = getRtcTime();
            setTime(gotTime);
            statusUpdateNtpTime = 0;
        }
    }
    Serial.println("ITime Ready RTC!");
}
#else
void timeSynch()
{
    time_t gotTime;
    if (WiFi.status() == WL_CONNECTED)
    {
        gotTime = getNtpTime();
        if (gotTime <= 0)
        {
            statusUpdateNtpTime = 0;
        }
        else
        {
            setTime(gotTime);
            Serial.println("timeSynch() Ready NTP!");
        }
    }
    else
    {
        statusUpdateNtpTime = 0;
    }
}
#endif

String GetTime(bool s)   //s - show seconds
{
    if ((timeStatus() != timeNotSet) && (timeStatus() != timeNeedsSync))
    {
        time_t tn = now();
        String Time;
        if (s)
        {
            Time = String(hour(tn)) + ":" + (minute(tn) < 10 ? "0" + String(minute(tn)) : String(minute(tn))) + ":" + String(second(tn));
        }
        else
        {
            Time = String(hour(tn)) + ":" + (minute(tn) < 10 ? "0" + String(minute(tn)) : String(minute(tn)));
        }
        //Serial.println("GetTime() "+Time);
        return Time;
    }
    else
    {
        timeSynch();
        return "";
    }
}

void getTimeDisp(char* psz, bool f)   // Code for reading clock time for displey only
{
    time_t tn = now();
    sprintf(psz, "%02d%c%02d", hour(tn), (f ? ':' : ' '), minute(tn));
}


String GetDate()
{
    time_t tn = now();
    //String Date = String(day(tn))+"."+(month(tn)<10 ? "0" + String(month(tn)) :  String(month(tn))) +"."+String(year(tn));//""; // Строка для результатов времени
    //String Date = String(day(tn))+" "+monthStr(month(tn)) +" "+String(year(tn)) + ", " + dayStr(weekday(tn));//""; // Строка для результатов времени
    String Date = String(day(tn)) + " " + month_table[lang][month(tn) - 1] + " " + String(year(tn)) + ", " + day_table[lang][weekday(tn) - 1];
    //String Date = String(day(tn))+ "." + (month(tn)<10 ? "0" + String(month(tn)) :  String(month(tn))) + "." + String(year(tn)) + ", " + day_table[lang][weekday(tn)-1];
    //Serial.println(Date);
    Date.toLowerCase();
    return Date;
}

bool compTimeInt(float tFrom, float tTo, float tNow)   //Comparing time for proper processing from 18.00 to 8.00
{
    if (tFrom < tTo)
    {
        if ((tFrom <= tNow) && (tTo >= tNow))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
        if (tFrom > tTo)
        {
            if (tNow <= 23.59 && tFrom <= tNow)
            {
                return true;
            }
            else
                if (tNow >= 0 && tTo >= tNow)
                {
                    return true;
                }
                else
                {
                    return false;
                }
        }
        else
        {
            return false;
        }
}
