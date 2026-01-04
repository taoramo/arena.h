#include "base.h"

# define STB_SPRINTF_IMPLEMENTATION
# define STB_SPRINTF_STATIC
# include "third_party/stb_sprintf.h"

internal U16
safe_cast_u16(U32 x)
{
  AssertAlways(x <= max_U16);
  U16 result = (U16)x;
  return result;
}

internal U32
safe_cast_u32(U64 x)
{
  AssertAlways(x <= max_U32);
  U32 result = (U32)x;
  return result;
}

internal S32
safe_cast_s32(S64 x)
{
  AssertAlways(x <= max_S32);
  S32 result = (S32)x;
  return result;
}

internal DenseTime
dense_time_from_date_time(DateTime date_time){
  DenseTime result = 0;
  result += date_time.year;
  result *= 12;
  result += date_time.mon;
  result *= 31;
  result += date_time.day;
  result *= 24;
  result += date_time.hour;
  result *= 60;
  result += date_time.min;
  result *= 61;
  result += date_time.sec;
  result *= 1000;
  result += date_time.msec;
  return(result);
}

internal DateTime
date_time_from_dense_time(DenseTime time){
  DateTime result = {0};
  result.msec = time%1000;
  time /= 1000;
  result.sec  = time%61;
  time /= 61;
  result.min  = time%60;
  time /= 60;
  result.hour = time%24;
  time /= 24;
  result.day  = time%31;
  time /= 31;
  result.mon  = time%12;
  time /= 12;
  Assert(time <= max_U32);
  result.year = (U32)time;
  return(result);
}

internal DateTime
date_time_from_micro_seconds(U64 time){
  DateTime result = {0};
  result.micro_sec = time%1000;
  time /= 1000;
  result.msec = time%1000;
  time /= 1000;
  result.sec = time%60;
  time /= 60;
  result.min = time%60;
  time /= 60;
  result.hour = time%24;
  time /= 24;
  result.day = time%31;
  time /= 31;
  result.mon = time%12;
  time /= 12;
  Assert(time <= max_U32);
  result.year = (U32)time;
  return(result);
}

internal DateTime
date_time_from_unix_time(U64 unix_time)
{
  DateTime date = {0};
  date.year     = 1970;
  date.day      = 1 + (unix_time / 86400);
  date.sec      = (U32)unix_time % 60;
  date.min      = (U32)(unix_time / 60) % 60;
  date.hour     = (U32)(unix_time / 3600) % 24;
  
  for(;;)
  {
    for(date.month = 0; date.month < 12; ++date.month)
    {
      U64 c = 0;
      switch(date.month)
      {
        case Month_Jan: c = 31; break;
        case Month_Feb:
        {
          if((date.year % 4 == 0) && ((date.year % 100) != 0 || (date.year % 400) == 0))
          {
            c = 29;
          }
          else
          {
            c = 28;
          }
        } break;
        case Month_Mar: c = 31; break;
        case Month_Apr: c = 30; break;
        case Month_May: c = 31; break;
        case Month_Jun: c = 30; break;
        case Month_Jul: c = 31; break;
        case Month_Aug: c = 31; break;
        case Month_Sep: c = 30; break;
        case Month_Oct: c = 31; break;
        case Month_Nov: c = 30; break;
        case Month_Dec: c = 31; break;
        default: InvalidPath;
      }
      if(date.day <= c)
      {
        goto exit;
      }
      date.day -= c;
    }
    ++date.year;
  }
  exit:;
  
  return date;
}

internal U64
u64_array_bsearch(U64 *arr, U64 count, U64 value)
{
  if(count > 1 && arr[0] <= value && value < arr[count-1])
  {
    U64 l = 0;
    U64 r = count - 1;
    for(; l <= r; )
    {
      U64 m = l + (r - l) / 2;
      if(arr[m] == value)
      {
        return m;
      }
      else if(arr[m] < value)
      {
        l = m + 1;
      }
      else
      {
        r = m - 1;
      }
    }
  }
  else if (count == 1 && arr[0] == value)
  {
    return 0;
  }
  return max_U64;
}
