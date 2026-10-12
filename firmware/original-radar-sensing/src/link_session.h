#ifndef ORIGINAL_LINK_SESSION_H
#define ORIGINAL_LINK_SESSION_H
#include <stdint.h>
#define OR_PROTOCOL "original-radar-v2"
#define OR_PAIR_NONCE_LENGTH 32U
#define OR_KEEPALIVE_INTERVAL_MS 1000U
#define OR_KEEPALIVE_LEASE_MS 5000U
typedef struct {
    char nonce[OR_PAIR_NONCE_LENGTH+1U];
    uint32_t config_generation,session_id;
    uint8_t probe_sent,paired,start_permit;
    uint8_t lease_active;
    uint32_t last_keepalive_ms;
} OrLinkSession;
int or_link_nonce_valid(const char *nonce);
void or_link_revoke(OrLinkSession *link);
int or_link_commit(OrLinkSession *link);
int or_link_probe(OrLinkSession *link,const char *nonce);
int or_link_confirm(OrLinkSession *link,const char *nonce);
int or_link_consume(OrLinkSession *link);
int or_link_lease_begin(OrLinkSession *link,uint32_t now_ms);
/* Arguments are nonce, config generation and session ID, separated by whitespace. */
int or_link_keepalive(OrLinkSession *link,const char *arguments,uint32_t now_ms);
int or_link_lease_expired(const OrLinkSession *link,uint32_t now_ms);
#endif
