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
    l.session_id=UINT32_MAX;assert(or_link_probe(&l,n)<0 && !l.paired);
    l.config_generation=UINT32_MAX;assert(or_link_commit(&l)<0);
    puts("PASS: strict nonce, probe/confirm, one-shot authorization, revocation and counter exhaustion");return 0;
}
