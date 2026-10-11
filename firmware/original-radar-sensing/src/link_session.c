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
    link->probe_sent=0U;link->paired=0U;link->start_permit=0U;
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
