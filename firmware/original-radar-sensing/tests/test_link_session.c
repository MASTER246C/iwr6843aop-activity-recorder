#include "link_session.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    OrLinkSession l={0};const char *n="0123456789abcdef0123456789abcdef";
    assert(!or_link_nonce_valid(0));assert(!or_link_nonce_valid(""));
    assert(!or_link_nonce_valid("0123456789abcdef0123456789abcdeF"));
    assert(!or_link_nonce_valid("0123456789abcdef0123456789abcdef0"));
    assert(or_link_nonce_valid(n));assert(or_link_consume(&l)<0);
    assert(or_link_commit(&l)==0 && l.config_generation==1U);
    assert(or_link_probe(&l,n)==0 && l.session_id==1U);
    assert(or_link_confirm(&l,n)<0);l.probe_sent=1;
    assert(or_link_confirm(&l,n)==0);assert(or_link_consume(&l)==0);
    assert(l.paired && !l.start_permit);assert(or_link_consume(&l)<0);
    assert(or_link_confirm(&l,n)<0 && !l.paired);
    assert(or_link_probe(&l,n)==0 && l.session_id==2U);l.probe_sent=1;
    assert(or_link_confirm(&l,n)==0);assert(or_link_commit(&l)==0);
    assert(!l.paired && !l.start_permit && !l.probe_sent && l.config_generation==2U);
    /* An explicit continuous run has a finite rolling control lease. */
    assert(or_link_probe(&l,n)==0);l.probe_sent=1;
    assert(or_link_confirm(&l,n)==0);
    assert(or_link_lease_begin(&l,100U)<0); /* Start has not consumed its permit. */
    assert(or_link_consume(&l)==0 && or_link_lease_begin(&l,100U)==0);
    assert(!or_link_lease_expired(&l,5099U));
    assert(or_link_lease_expired(&l,5100U));
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 3",5100U)<0);
    assert(l.last_keepalive_ms==100U); /* An expired lease cannot be resurrected. */
    assert(or_link_lease_begin(&l,100U)==0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 3",1100U)==0);
    assert(l.last_keepalive_ms==1100U);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdee 2 3",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 1 3",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 2",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 3 extra",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 4294967296 3",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef -2 3",1200U)<0);
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2",1200U)<0);
    assert(l.last_keepalive_ms==1100U);
    assert(or_link_keepalive(&l," 0123456789abcdef0123456789abcdef\t2\t3\r",2100U)==0);
    assert(or_link_lease_begin(&l,UINT32_MAX-1000U)==0);
    assert(!or_link_lease_expired(&l,1000U));
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 3",1000U)==0);
    assert(!or_link_lease_expired(&l,5999U) && or_link_lease_expired(&l,6000U));
    or_link_revoke(&l);
    assert(!l.lease_active && !or_link_lease_expired(&l,6000U));
    assert(or_link_keepalive(&l,"0123456789abcdef0123456789abcdef 2 3",6000U)<0);
    l.session_id=UINT32_MAX;assert(or_link_probe(&l,n)<0 && !l.paired);
    l.config_generation=UINT32_MAX;assert(or_link_commit(&l)<0);
    puts("PASS: strict nonce, one-shot authorization, correlated rolling lease, wrap, revocation and counter exhaustion");return 0;
}
