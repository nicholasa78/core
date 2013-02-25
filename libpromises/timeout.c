/* 
   Copyright (C) Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License  
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of Cfengine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.

*/

#include "cf3.defs.h"

#include "env_context.h"
#include "cfstream.h"

/* Prototypes */

static void AddTimeClass(EvalContext *ctx, time_t time);
static void RemoveTimeClass(EvalContext *ctx, time_t time);

/*************************************************************************/

void SetTimeOut(int timeout)
{
    ALARM_PID = -1;
    signal(SIGALRM, (void *) TimeOut);
    alarm(timeout);
}

/*************************************************************************/

void TimeOut()
{
    alarm(0);

    if (ALARM_PID != -1)
    {
        CfOut(OUTPUT_LEVEL_VERBOSE, "", "Time out of process %jd\n", (intmax_t)ALARM_PID);
        GracefulTerminate(ALARM_PID);
    }
    else
    {
        CfOut(OUTPUT_LEVEL_VERBOSE, "", "%s> Time out\n", VPREFIX);
    }
}

/*************************************************************************/

void SetReferenceTime(EvalContext *ctx, int setclasses)
{
    time_t tloc;
    char vbuff[CF_BUFSIZE];

    if ((tloc = time((time_t *) NULL)) == -1)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "time", "Couldn't read system clock\n");
    }

    CFSTARTTIME = tloc;

    snprintf(vbuff, CF_BUFSIZE, "%s", cf_ctime(&tloc));

    CfOut(OUTPUT_LEVEL_VERBOSE, "", "Reference time set to %s\n", cf_ctime(&tloc));

    if (setclasses)
    {
        RemoveTimeClass(ctx, tloc);
        AddTimeClass(ctx, tloc);
    }
}

/*******************************************************************/

void SetStartTime(void)
{
    time_t tloc;

    if ((tloc = time((time_t *) NULL)) == -1)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "time", "Couldn't read system clock\n");
    }

    CFINITSTARTTIME = tloc;

    CfDebug("Job start time set to %s\n", cf_ctime(&tloc));
}

/*********************************************************************/

static void RemoveTimeClass(EvalContext *ctx, time_t time)
{
    int i, j;
    struct tm parsed_time;
    char buf[CF_BUFSIZE];

    if (localtime_r(&time, &parsed_time) == NULL)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "localtime_r", "Unable to parse passed time");
        return;
    }

/* Lifecycle */

    for( i = 0; i < 3; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "Lcycle_%d", i);
        DeleteHardClass(ctx, buf);
    }

/* Year */

    snprintf(buf, CF_BUFSIZE, "Yr%04d", parsed_time.tm_year - 1 + 1900);
    DeleteHardClass(ctx, buf);
    snprintf(buf, CF_BUFSIZE, "Yr%04d", parsed_time.tm_year + 1900);
    DeleteHardClass(ctx, buf);

/* Month */

    for( i = 0; i < 12; i++ )
    {
        DeleteHardClass(ctx, MONTH_TEXT[i]);
    }

/* Day of week */

    for( i = 0; i < 7; i++ )
    {
        DeleteHardClass(ctx, DAY_TEXT[i]);
    }

/* Day */

    for( i = 1; i < 32; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "Day%d", i);
        DeleteHardClass(ctx, buf);
    }

/* Shift */

    for( i = 0; i < 4; i++ )
    {
        DeleteHardClass(ctx, SHIFT_TEXT[i]);
    }

/* Hour */

    for( i = 0; i < 24; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "Hr%02d", i);
        DeleteHardClass(ctx, buf);
    }

/* GMT hour */

    for( i = 0; i < 24; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "GMT_Hr%02d", i);
        DeleteHardClass(ctx, buf);
    }

/* Quarter */

    for( i = 1; i <= 4; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "Q%d", i);
        DeleteHardClass(ctx, buf);
        for( j = 0; j < 24; j++ )
        {
            snprintf(buf, CF_BUFSIZE, "Hr%02d_Q%d", j, i);
            DeleteHardClass(ctx, buf);
        }
    }

/* Minute */

    for( i = 0; i < 60; i++ )
    {
        snprintf(buf, CF_BUFSIZE, "Min%02d", i);
        DeleteHardClass(ctx, buf);
    }

    for( i = 0; i < 60; i += 5 )
    {
        snprintf(buf, CF_BUFSIZE, "Min%02d_%02d", i, (i + 5) % 60);
        DeleteHardClass(ctx, buf);
    }
}

/*********************************************************************/

static void AddTimeClass(EvalContext *ctx, time_t time)
{
    struct tm parsed_time;
    struct tm gmt_parsed_time;
    char buf[CF_BUFSIZE];
    int day_text_index, quarter, interval_start, interval_end;

    if (localtime_r(&time, &parsed_time) == NULL)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "localtime_r", "Unable to parse passed time");
        return;
    }

    if (gmtime_r(&time, &gmt_parsed_time) == NULL)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "gmtime_r", "Unable to parse passed date");
        return;
    }

/* Lifecycle */

    snprintf(buf, CF_BUFSIZE, "Lcycle_%d", ((parsed_time.tm_year + 1900) % 3));
    HardClass(ctx, buf);

/* Year */

    snprintf(VYEAR, CF_BUFSIZE, "%04d", parsed_time.tm_year + 1900);
    snprintf(buf, CF_BUFSIZE, "Yr%04d", parsed_time.tm_year + 1900);
    HardClass(ctx, buf);

/* Month */

    strlcpy(VMONTH, MONTH_TEXT[parsed_time.tm_mon], 4);
    HardClass(ctx, MONTH_TEXT[parsed_time.tm_mon]);

/* Day of week */

/* Monday  is 1 in tm_wday, 0 in DAY_TEXT
   Tuesday is 2 in tm_wday, 1 in DAY_TEXT
   ...
   Sunday  is 0 in tm_wday, 6 in DAY_TEXT */
    day_text_index = (parsed_time.tm_wday + 6) % 7;
    HardClass(ctx, DAY_TEXT[day_text_index]);

/* Day */

    snprintf(VDAY, CF_BUFSIZE, "%d", parsed_time.tm_mday);
    snprintf(buf, CF_BUFSIZE, "Day%d", parsed_time.tm_mday);
    HardClass(ctx, buf);

/* Shift */

    strcpy(VSHIFT, SHIFT_TEXT[parsed_time.tm_hour / 6]);
    HardClass(ctx, VSHIFT);

/* Hour */

    snprintf(buf, CF_BUFSIZE, "Hr%02d", parsed_time.tm_hour);
    HardClass(ctx, buf);

/* GMT hour */

    snprintf(buf, CF_BUFSIZE, "GMT_Hr%d\n", gmt_parsed_time.tm_hour);
    HardClass(ctx, buf);

/* Quarter */

    quarter = parsed_time.tm_min / 15 + 1;

    snprintf(buf, CF_BUFSIZE, "Q%d", quarter);
    HardClass(ctx, buf);
    snprintf(buf, CF_BUFSIZE, "Hr%02d_Q%d", parsed_time.tm_hour, quarter);
    HardClass(ctx, buf);

/* Minute */

    snprintf(buf, CF_BUFSIZE, "Min%02d", parsed_time.tm_min);
    HardClass(ctx, buf);

    interval_start = (parsed_time.tm_min / 5) * 5;
    interval_end = (interval_start + 5) % 60;

    snprintf(buf, CF_BUFSIZE, "Min%02d_%02d", interval_start, interval_end);
    HardClass(ctx, buf);
}

/*********************************************************************/

#ifndef __MINGW32__

bool IsReadReady(int fd, int timeout_sec)
{
    fd_set  rset;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    struct timeval tv = {
        .tv_sec = timeout_sec,
        .tv_usec = 0,
    };

    int ret = select(fd + 1, &rset, NULL, NULL, &tv);

    if(ret < 0)
    {
        CfOut(OUTPUT_LEVEL_ERROR, "select", "!! IsReadReady: Failed checking for data");
        return false;
    }

    if(FD_ISSET(fd, &rset))
    {
        return true;
    }

    if(ret == 0)  // timeout
    {
        return false;
    }

    // can we get here?
    CfOut(OUTPUT_LEVEL_ERROR, "select", "!! IsReadReady: Unknown outcome (ret > 0 but our only fd is not set)");

    return false;
}

#endif  /* __MINGW32__ */
