#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "managers/lab_poc.h"
#include "managers/lab_poc_html.h"

static lab_poc_fixture_t parse(const char *s) {
    return lab_poc_parse_form(s, strlen(s));
}

int main(void) {
    assert(parse("fixture=paypal-sandbox") == LAB_POC_PAYPAL_SANDBOX);
    assert(parse("fixture=card-test-token") == LAB_POC_CARD_TEST_TOKEN);
    assert(parse("fixture=paypal-sandbox&password=secret") == LAB_POC_INVALID);
    assert(parse("password=secret") == LAB_POC_INVALID);
    assert(parse("fixture=card-test-token&card=4111111111111111") == LAB_POC_INVALID);
    assert(parse("fixture=paypal-sandbox%00") == LAB_POC_INVALID);
    assert(parse("fixture=paypal-sandbox&fixture=card-test-token") == LAB_POC_INVALID);
    assert(lab_poc_parse_form("fixture=paypal-sandbox\0extra", 28) == LAB_POC_INVALID);
    assert(strstr(lab_poc_html, "type=\"text\"") == NULL);
    assert(strstr(lab_poc_html, "type=\"password\"") == NULL);
    assert(strstr(lab_poc_html, "<script") == NULL);
    puts("Lab PoC: fixed fixtures only; no free-text or keystroke capture");
}
