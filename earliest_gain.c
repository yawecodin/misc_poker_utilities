#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define YEAR_IX  0
#define MONTH_IX 1
#define DAY_IX   2

#define MAX_LINE_LEN 1024
static char line[MAX_LINE_LEN];

#define TAB 0x9

static char usage[] =
"usage: earliest_gain amount filename\n";
static char couldnt_open[] = "couldn't open %s\n";

static char malloc_failed1[] = "malloc of %d session info structures failed\n";
static char malloc_failed2[] = "malloc of %d ints failed\n";

static char fmt1[] = "%10d %4d ";
static char fmt2[] = "%10d %4d %10.2lf (%d)\n";

struct digit_range {
  int lower;
  int upper;
};

static struct digit_range date_checks[3] = {
  80, 2095,  /* year */
  1, 12,     /* month */
  1, 31     /* day */
};

static char *months[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
#define NUM_MONTHS (sizeof months / sizeof (char *))

struct session_info_struct {
  int starting_amount;
  int starting_ix;
  int ending_amount;
  int gain_amount;
  int num_gain_sessions;
  int winning_session;
  int num_winning_sessions;
  time_t gain_start_date;
  time_t gain_end_date;
};

static struct session_info_struct *session_info;

static void GetLine(FILE *fptr,char *line,int *line_len,int maxllen);
static int get_session_info(
  char *line,
  int line_len,
  struct session_info_struct *session_info);
static time_t cvt_date(char *date_str);
static char *format_date(char *cpt);

int main(int argc,char **argv)
{
  int m;
  int n;
  int p;
  int gain_threshold;
  FILE *fptr;
  int line_len;
  int num_sessions;
  int num_winning_sessions;
  int ix;
  int retval;
  char *cpt;
  double avg_amount;

  if (argc != 3) {
    printf(usage);
    return 1;
  }

  sscanf(argv[1],"%d",&gain_threshold);

  if ((fptr = fopen(argv[2],"r")) == NULL) {
    printf(couldnt_open,argv[2]);
    return 2;
  }

  num_sessions = 0;

  for ( ; ; ) {
    GetLine(fptr,line,&line_len,MAX_LINE_LEN);

    if (feof(fptr))
      break;

    num_sessions++;
  }

  fseek(fptr,0L,SEEK_SET);

  if ((session_info = (struct session_info_struct *)malloc(
    num_sessions * sizeof (struct session_info_struct))) == NULL) {
    printf(malloc_failed1,num_sessions);
    fclose(fptr);
    return 3;
  }

  ix = 0;

  for ( ; ; ) {
    GetLine(fptr,line,&line_len,MAX_LINE_LEN);

    if (feof(fptr))
      break;

    retval = get_session_info(line,line_len,&session_info[ix]);

    session_info[ix].num_gain_sessions = -1;
    session_info[ix].starting_ix = ix;

    ix++;
  }

  fclose(fptr);

  for (m = 0; m < num_sessions; m++) {
    for (n = m; n < num_sessions; n++) {
      if (session_info[n].ending_amount - session_info[m].starting_amount
        >= gain_threshold) {

        num_winning_sessions = 0;

        for (p = m; p <= n; p++)
          num_winning_sessions += session_info[p].winning_session;

        session_info[m].num_gain_sessions = n - m + 1;
        session_info[m].num_winning_sessions = num_winning_sessions;
        session_info[m].gain_amount =
          session_info[n].ending_amount - session_info[m].starting_amount;
        session_info[m].gain_end_date = session_info[n].gain_start_date;

        printf(fmt1,
          session_info[m].starting_amount,
          session_info[m].starting_ix);

        cpt = ctime(&session_info[m].gain_start_date);
        printf("%s\n",format_date(cpt));

        printf(fmt1,
          session_info[m].starting_amount +
            session_info[m].gain_amount,
          session_info[m].starting_ix +
            session_info[m].num_gain_sessions - 1);

        cpt = ctime(&session_info[m].gain_end_date);
        printf("%s\n",format_date(cpt));

        avg_amount = (double)session_info[m].gain_amount /
          (double)session_info[m].num_gain_sessions;

        printf(fmt2,
          session_info[m].gain_amount,
          session_info[m].num_gain_sessions,
          avg_amount,
          session_info[m].num_winning_sessions);

        break;
      }
    }

    if (n < num_sessions)
      break;
  }

  free(session_info);

  return 0;
}

static void GetLine(FILE *fptr,char *line,int *line_len,int maxllen)
{
  int chara;
  int local_line_len;

  local_line_len = 0;

  for ( ; ; ) {
    chara = fgetc(fptr);

    if (feof(fptr))
      break;

    if (chara == '\n')
      break;

    if (local_line_len < maxllen - 1)
      line[local_line_len++] = (char)chara;
  }

  line[local_line_len] = 0;
  *line_len = local_line_len;
}

static int get_session_info(
  char *line,
  int line_len,
  struct session_info_struct *session_info)
{
  int m;
  int n;
  int work;

  for (n = 0; n < line_len; n++) {
    if (line[n] == TAB)
      break;
  }

  if (n == line_len)
    return 1;

  line[n++] = 0;

  session_info->gain_start_date = cvt_date(line);

  for (m = n; n < line_len; n++) {
    if (line[n] == TAB)
      break;
  }

  if (n == line_len)
    return 2;

  line[n++] = 0;

  sscanf(&line[m],"%d",&work);

  session_info->starting_amount = work;

  sscanf(&line[n],"%d",&work);

  session_info->ending_amount = work;

  if (session_info->ending_amount > session_info->starting_amount)
    session_info->winning_session = 1;
  else
    session_info->winning_session = 0;

  return 0;
}

static time_t cvt_date(char *date_str)
{
  struct tm tim;
  char hold[11];
  int date_len;
  int bufix;
  int holdix;
  int digits[3];
  int n;
  time_t ret_tm;

  date_len = strlen(date_str);

  if (!date_len || (date_len > 10))
    return -1L;

  bufix = 0;

  for (n = 0; n < 3; n++) {
    holdix = 0;

    for ( ; bufix < date_len; ) {
      if (date_str[bufix] == '-') {
        bufix++;
        break;
      }

      if ((date_str[bufix] < '0') || (date_str[bufix] > '9'))
        return -1L;

      hold[holdix++] = date_str[bufix++];
    }

    if (!holdix || ((n != 2) && (bufix == date_len)))
      return -1L;

    hold[holdix] = 0;
    digits[n] = atoi(hold);

    if ((digits[n] > date_checks[n].upper) ||
      (digits[n] < date_checks[n].lower))
      return -1L;
  }

  if (digits[YEAR_IX] >= 100)
    if (digits[YEAR_IX] < 1970)
      return -1L;
    else
      digits[YEAR_IX] -= 1900;

  tim.tm_mon = digits[MONTH_IX] - 1;
  tim.tm_mday = digits[DAY_IX];
  tim.tm_year = digits[YEAR_IX];

  tim.tm_hour = 0;
  tim.tm_min = 0;
  tim.tm_sec = 0;

  tim.tm_isdst = 0;

  ret_tm = mktime(&tim);

  return ret_tm;
}

static char *format_date(char *cpt)
{
  int month;
  static char date_buf[11];

  cpt[7] = 0;
  cpt[10] = 0;
  cpt[24] = 0;

  for (month = 0; month < NUM_MONTHS; month++) {
    if (!strcmp(&cpt[4],months[month]))
      break;
  }

  if (month == NUM_MONTHS)
    month = 0;

  if (cpt[8] == ' ')
    cpt[8] = '0';

  sprintf(date_buf,"%s-%02d-%s",&cpt[20],month+1,&cpt[8]);

  return date_buf;
}
