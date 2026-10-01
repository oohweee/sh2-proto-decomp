/*
 * sh2_scf.c: the game's view of the console settings (language, date/time
 * notation, time zone), and date/time string formatting.
 */

#include "sh2.h"
#include "libc/stdio.h"

static struct sceScfT10kConfig sh2ScfInfo = { 0 };

/** Reads the console settings (time zone, aspect, language, notations, ...) into sh2ScfInfo. */
void sh2ScfInit(void) {
    ps2ScfInit();
    sh2ScfInfo.TimeZone = ps2ScfGetTimeZone();
    sh2ScfInfo.Aspect = ps2ScfGetAspect();
    sh2ScfInfo.DateNotation = ps2ScfGetDateNotation();
    sh2ScfInfo.Language = ps2ScfGetLanguage();
    sh2ScfInfo.Spdif = ps2ScfGetSpdif();
    sh2ScfInfo.SummerTime = ps2ScfGetSummerTime();
    sh2ScfInfo.TimeNotation = ps2ScfGetTimeNotation();
}

/** Maps the console language (SCE_*_LANGUAGE) to the game's language. */
enum SPEC_LANG_O sh2ScfGetDefaultLanguage(void) {
    switch (sh2ScfInfo.Language) {
    case 1: /* English */
    case 6: /* Dutch */
    case 7: /* Portuguese */
    default:
        return SPEC_LANG_ENG_O;
    case 0: /* Japanese */
        return SPEC_LANG_JPN_O;
    case 2: /* French */
        return SPEC_LANG_FRN_O;
    case 3: /* Spanish */
        return SPEC_LANG_SPN_O;
    case 4: /* German */
        return SPEC_LANG_GER_O;
    case 5: /* Italian */
        return SPEC_LANG_ITA_O;
    }
}

/** Returns the BCD number `bcd` as binary. */
unsigned int UtilBCDToDecimal(unsigned int bcd) {
    unsigned int dec;
    int keta;

    for (dec = 0, keta = 1; bcd != 0; bcd >>= 4, keta *= 10) {
        dec += (bcd & 0xF) * keta;
    }
    return dec;
}

/**
 * Writes the date `year`/`month`/`day` into `datestr` in the console's date notation. Returns the
 * length.
 */
int sh2ScfMakeDateStr(char *datestr, unsigned int year, unsigned int month, unsigned int day) {
    switch (sh2ScfInfo.DateNotation) {
    case 0: /* YYYYMMDD */
        sprintf(datestr, "%04d/%02d/%02d", year, month, day);
        break;
    case 1: /* MMDDYYYY */
        sprintf(datestr, "%02d/%02d/%04d", month, day, year);
        break;
    case 2: /* DDMMYYYY */
        sprintf(datestr, "%02d/%02d/%04d", day, month, year);
        break;
    }
    return 10;
}

/**
 * Writes the date of the RTC time `jst`, converted to local time, into `datestr`. Returns the
 * length.
 */
int sh2ScfMakeDateStrByLocalTimeFromJST(char *datestr, struct sceCdCLOCK *jst) {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    struct sceCdCLOCK rtc[1];

    rtc[0] = *jst;
    ps2ScfGetLocalTimefromRTC(rtc);
    year = UtilBCDToDecimal(rtc[0].year) + 2000;
    month = UtilBCDToDecimal(rtc[0].month);
    day = UtilBCDToDecimal(rtc[0].day);
    /* the local date crossed a century boundary */
    if (rtc[0].year == 0x99 && jst->year == 0x00) {
        year -= 100;
    }
    if (rtc[0].year == 0x00 && jst->year == 0x99) {
        year += 100;
    }
    return sh2ScfMakeDateStr(datestr, year, month, day);
}

/**
 * Writes the time `hour24`:`minute`:`second` into `timestr` in the console's 24- or 12-hour
 * notation. Returns the length.
 */
int sh2ScfMakeTimeStr(char *timestr, unsigned int hour24, unsigned int minute, unsigned int second) {
    if (sh2ScfInfo.TimeNotation == 0) {
        sprintf(timestr, "%02d:%02d:%02d", hour24, minute, second);
        return 8;
    } else {
        sprintf(timestr, "%2d:%02d:%02d %cM", (hour24 + 11) % 12 + 1, minute, second, (hour24 < 12) ? 'A' : 'P');
        return 12;
    }
}

/** Writes the time of RTC value `rtc` into `timestr`. Returns the length. */
int sh2ScfMakeTimeStrFromRTC(char *timestr, struct sceCdCLOCK *rtc) {
    unsigned int hour;
    unsigned int minute;
    unsigned int second;

    hour = UtilBCDToDecimal(rtc->hour);
    minute = UtilBCDToDecimal(rtc->minute);
    second = UtilBCDToDecimal(rtc->second);
    return sh2ScfMakeTimeStr(timestr, hour, minute, second);
}

/**
 * Writes the time of the RTC time `jst`, converted to local time, into `datestr`. Returns the
 * length.
 */
int sh2ScfMakeTimeStrByLocalTimeFromJST(char *datestr, struct sceCdCLOCK *jst) {
    struct sceCdCLOCK rtc[1];

    rtc[0] = *jst;
    ps2ScfGetLocalTimefromRTC(rtc);
    return sh2ScfMakeTimeStrFromRTC(datestr, rtc);
}
