/* Original bounded cross-port correlation. This is not authentication. */
#include "link_session.h"
#include <string.h>
#include <limits.h>
int or_link_nonce_valid(const char *nonce)
{
    unsigned i;if(nonce==0)return 0;
    for(i=0U;i<OR_PAIR_NONCE_LENGTH;++i) {
        char c=nonce[i];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return 0;
    }
    return nonce[OR_PAIR_NONCE_LENGTH]=='\0';
}
void or_link_revoke(OrLinkSession *link)
{
    link->probe_sent=0U;link->paired=0U;link->start_permit=0U;link->lease_active=0U;
}
int or_link_commit(OrLinkSession *link)
{
    or_link_revoke(link);
    if(link->config_generation==UINT32_MAX)return -1;
    ++link->config_generation;return 0;
}
int or_link_probe(OrLinkSession *link,const char *nonce)
{
    or_link_revoke(link);
    if(!or_link_nonce_valid(nonce)||link->session_id==UINT32_MAX)return -1;
    memcpy(link->nonce,nonce,OR_PAIR_NONCE_LENGTH+1U);++link->session_id;
    return 0;
}
int or_link_confirm(OrLinkSession *link,const char *nonce)
{
    if(!link->probe_sent||!or_link_nonce_valid(nonce)||strcmp(link->nonce,nonce)!=0) {
        or_link_revoke(link);return -1;
    }
    link->probe_sent=0U;link->paired=1U;link->start_permit=1U;return 0;
}
int or_link_consume(OrLinkSession *link)
{
    if(!link->paired||!link->start_permit)return -1;
    link->start_permit=0U;return 0;
}
int or_link_lease_begin(OrLinkSession *link,uint32_t now_ms)
{
    if(!link->paired || link->start_permit)return -1;
    link->last_keepalive_ms=now_ms;link->lease_active=1U;return 0;
}
int or_link_lease_expired(const OrLinkSession *link,uint32_t now_ms)
{
    return link->lease_active &&
        (uint32_t)(now_ms-link->last_keepalive_ms)>=OR_KEEPALIVE_LEASE_MS;
}
static int whitespace(char c){return c==' ' || c=='\t' || c=='\r';}
static int decimal_u32(const char **cursor,uint32_t *out)
{
    const char *p=*cursor;uint32_t value=0U;
    while(whitespace(*p))++p;
    if(*p<'0' || *p>'9')return 0;
    do {
        uint32_t digit=(uint32_t)(*p-'0');
        if(value>(UINT32_MAX-digit)/10U)return 0;
        value=value*10U+digit;++p;
    }while(*p>='0' && *p<='9');
    if(*p!='\0' && !whitespace(*p))return 0;
    *cursor=p;*out=value;return 1;
}
int or_link_keepalive(OrLinkSession *link,const char *arguments,uint32_t now_ms)
{
    char nonce[OR_PAIR_NONCE_LENGTH+1U];const char *p=arguments;
    uint32_t generation,session;unsigned i;
    if(p==NULL || !link->paired || !link->lease_active ||
       or_link_lease_expired(link,now_ms))return -1;
    while(whitespace(*p))++p;
    for(i=0U;i<OR_PAIR_NONCE_LENGTH;++i) {
        if(*p=='\0' || whitespace(*p))return -1;
        nonce[i]=*p++;
    }
    nonce[OR_PAIR_NONCE_LENGTH]='\0';
    if(!whitespace(*p) || !or_link_nonce_valid(nonce) ||
       !decimal_u32(&p,&generation) || !whitespace(*p) ||
       !decimal_u32(&p,&session))return -1;
    while(whitespace(*p))++p;
    if(*p!='\0' || strcmp(nonce,link->nonce)!=0 ||
       generation!=link->config_generation || session!=link->session_id)return -1;
    link->last_keepalive_ms=now_ms;return 0;
}
