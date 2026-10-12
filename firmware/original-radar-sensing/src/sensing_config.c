/* Original, bounded parser. No TI CLI dependency; RF fields are a locked profile. */
#include "sensing_config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#define OR_REQUIRED_FIELDS 15U

void or_config_reset(OrConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->bracket_top_mm=2000; cfg->down_tilt_mdeg=15000;
    cfg->session_seconds=5; cfg->first_range_bin=4; cfg->last_range_bin=30;
    cfg->minimum_power=64; cfg->noise_multiplier=8;
}
static int integer(const char *s, long low, long high, long *out)
{
    char *end; long v;
    if (*s=='\0' || *s=='+' || (*s!='-' && (*s<'0'||*s>'9'))) return 0;
    errno=0; v=strtol(s,&end,10);
    if (errno==ERANGE || *end!='\0' || v<low || v>high) return 0;
    *out=v; return 1;
}
int or_config_valid(const OrConfig *cfg)
{
    return cfg!=NULL && cfg->fields_seen==OR_REQUIRED_FIELDS &&
        cfg->bracket_top_mm>=100 && cfg->bracket_top_mm<=5000 &&
        cfg->down_tilt_mdeg>=-45000 && cfg->down_tilt_mdeg<=45000 &&
        cfg->antenna_vertical_offset_mm>=-1000 && cfg->antenna_vertical_offset_mm<=1000 &&
        (!cfg->antenna_offset_known || cfg->bracket_top_mm+cfg->antenna_vertical_offset_mm>0) &&
        cfg->first_range_bin>=1U && cfg->first_range_bin<cfg->last_range_bin &&
        cfg->last_range_bin<OR_ADC_SAMPLES/2U && cfg->minimum_power>=1U &&
        cfg->minimum_power<=1000000000U && cfg->noise_multiplier>=1U &&
        cfg->noise_multiplier<=1000U && cfg->session_seconds<=5U;
}
OrCommand or_config_line(OrConfig *cfg, const char *line, int running)
{
    char b[OR_LINE_SIZE]; char *v[8]; size_t n=0,i=0,j=0; long a,c,d,e;
    OrConfig next;
    if (cfg==NULL || line==NULL) return OR_CMD_INVALID;
    while (n<sizeof(b) && line[n]!='\0') ++n;
    if (n>=sizeof(b)) return OR_CMD_INVALID;
    memcpy(b,line,n+1U); n=0;
    while (b[i]!='\0') {
        while (b[i]==' '||b[i]=='\t'||b[i]=='\r'||b[i]=='\n') ++i;
        if (b[i]=='\0'||b[i]=='#') break;
        if (n>=8U) return OR_CMD_INVALID;
        v[n++]=&b[i];
        while (b[i]!='\0' && b[i]!=' '&&b[i]!='\t'&&b[i]!='\r'&&b[i]!='\n') {
            if ((unsigned char)b[i]<33U || (unsigned char)b[i]>126U) return OR_CMD_INVALID;
            ++i;
        }
        if (b[i]!='\0') b[i++]='\0';
    }
    if (n==0U) return OR_CMD_NONE;
    if(n==1U && strcmp(v[0],"stop")==0) return OR_CMD_STOP;
    if(n==1U && strcmp(v[0],"status")==0) return OR_CMD_STATUS;
    if(running) return OR_CMD_BUSY;
    if(n==1U && strcmp(v[0],"start")==0)
        return cfg->committed && or_config_valid(cfg)?OR_CMD_START:OR_CMD_INCOMPLETE;
    next=*cfg;
    if(n==2U && strcmp(v[0],"orConfig")==0 && strcmp(v[1],"1")==0) {
        or_config_reset(&next); j=1U;
    } else if(n==2U && strcmp(v[0],"profile")==0 && strcmp(v[1],"room64_v1")==0) {
        next.fields_seen|=1U; j=1U;
    } else if(n==4U && strcmp(v[0],"mount")==0 &&
              integer(v[1],100,5000,&a) && integer(v[2],-45000,45000,&c)) {
        if(strcmp(v[3],"unknown")==0) { next.antenna_offset_known=0U; d=0; }
        else if(integer(v[3],-1000,1000,&d)) next.antenna_offset_known=1U;
        else return OR_CMD_INVALID;
        if(next.antenna_offset_known && a+d<=0)return OR_CMD_INVALID;
        next.bracket_top_mm=(int32_t)a; next.down_tilt_mdeg=(int32_t)c;
        next.antenna_vertical_offset_mm=(int32_t)d; next.fields_seen|=2U; j=1U;
    } else if(n==5U && strcmp(v[0],"detect")==0 &&
        integer(v[1],1,30,&a)&&integer(v[2],2,31,&c)&&a<c&&
        integer(v[3],1,1000000000,&d)&&integer(v[4],1,1000,&e)) {
        next.first_range_bin=(uint16_t)a; next.last_range_bin=(uint16_t)c;
        next.minimum_power=(uint32_t)d; next.noise_multiplier=(uint32_t)e;
        next.fields_seen|=4U; j=1U;
    } else if(n==2U && strcmp(v[0],"session")==0 &&
              (strcmp(v[1],"continuous")==0 || integer(v[1],1,5,&a))) {
        next.session_seconds=strcmp(v[1],"continuous")==0?0U:(uint16_t)a;
        next.fields_seen|=8U; j=1U;
    } else if(n==1U && strcmp(v[0],"commit")==0) {
        if(!or_config_valid(&next)) return OR_CMD_INCOMPLETE;
        next.committed=1U; *cfg=next; return OR_CMD_COMMIT;
    }
    if(!j) return OR_CMD_INVALID;
    next.committed=0U; *cfg=next; return OR_CMD_CHANGED;
}
