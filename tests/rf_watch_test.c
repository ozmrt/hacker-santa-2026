#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "scans/rf_watch.h"

int main(void) {
    rf_watch_table_t table;
    rf_watch_init(&table);
    assert(rf_watch_validate(&table) && table.count == 0);
    assert(rf_watch_add(&table, RF_WATCH_AP_SSID, "Lab Signage"));
    assert(!rf_watch_add(&table, RF_WATCH_AP_SSID, "Lab Signage"));
    assert(rf_watch_add(&table, RF_WATCH_AP_BSSID, "AA:BB:CC:00:11:22"));
    assert(rf_watch_add(&table, RF_WATCH_BLE_NAME, "DisplayBeacon"));
    assert(rf_watch_add(&table, RF_WATCH_BLE_ADDR, "01:23:45:67:89:AB"));
    assert(!rf_watch_add(&table, RF_WATCH_BLE_ADDR, "01:23:45:67:89:GG"));
    assert(!rf_watch_add(&table, RF_WATCH_BLE_NAME, "A\nB"));
    uint8_t ssid[33] = "Lab Signage";
    uint8_t ap_mac[6] = {0xAA, 0xBB, 0xCC, 0, 0x11, 0x22};
    uint8_t ble_mac[6] = {1, 0x23, 0x45, 0x67, 0x89, 0xAB};
    assert(rf_watch_ap_matches(&table.rules[0], ssid, ap_mac));
    ssid[11] = 'X';
    assert(!rf_watch_ap_matches(&table.rules[0], ssid, ap_mac));
    assert(rf_watch_ap_matches(&table.rules[1], ssid, ap_mac));
    assert(rf_watch_ble_matches(&table.rules[2], "DisplayBeacon", ble_mac));
    assert(!rf_watch_ble_matches(&table.rules[2], "OtherBeacon", ble_mac));
    assert(rf_watch_ble_matches(&table.rules[3], "OtherBeacon", ble_mac));
    assert(rf_watch_remove(&table, 1));
    assert(table.count == 3 && table.rules[1].kind == RF_WATCH_BLE_NAME);
    assert(!rf_watch_remove(&table, 8));
    table.version = 2;
    assert(!rf_watch_validate(&table));
    puts("RF Watch: exact AP/BLE matches, validation and bounded rule table");
}
