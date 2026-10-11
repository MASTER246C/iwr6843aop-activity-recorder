#ifndef ORIGINAL_LINK_SESSION_H
#define ORIGINAL_LINK_SESSION_H
#include <stdint.h>
#define OR_PROTOCOL "original-radar-v2"
#define OR_PAIR_NONCE_LENGTH 32U
typedef struct {
    char nonce[OR_PAIR_NONCE_LENGTH+1U];
    uint32_t config_generation,session_id;
    uint8_t probe_sent,paired,start_permit;
} OrLinkSession;
int or_link_nonce_valid(const char *nonce);
void or_link_revoke(OrLinkSession *link);
int or_link_commit(OrLinkSession *link);
int or_link_probe(OrLinkSession *link,const char *nonce);
int or_link_confirm(OrLinkSession *link,const char *nonce);
int or_link_consume(OrLinkSession *link);
#endif
