# RF Watch: nearby device leads

`rfwatch` is a board-specific, passive report for known Wi-Fi access points and
BLE advertisers. It compares **exact** SSIDs, BSSIDs, BLE names and BLE
addresses against scan results already collected by GhostESP. Up to eight rules
are saved in NVS, so they survive a power cycle. The matcher has no heap
allocations or background task. Use the on-device Terminal (Apps → Tools) or
USB console.

Add identifying values from a signage board you own or have documented. A
product category such as “digital signage” has no universal radio signature:
SSID and BLE names are configurable and MAC addresses may change. A match is
an observation of the advertised identifier, not proof of device identity.

```text
rfwatch add ap ssid Lab Signage
rfwatch add ap bssid AA:BB:CC:00:11:22
rfwatch add ble name DisplayBeacon
rfwatch add ble addr 01:23:45:67:89:AB
rfwatch list
```

Names are exact, case-sensitive matches. BLE names are limited to 23 printable
ASCII characters because the upstream advertiser result truncates longer names.
Use `rfwatch remove <index>` or `rfwatch clear` to change the persistent list.
There is no default signage vendor database in this port.

Run each existing scanner separately, then report after stopping it:

```text
scanap
blescan -adv
blescan -s
bledetect
bledetect -s
rfwatch report
```

The report gives observed BSSID/address, RSSI and the matching rule index. It
also includes upstream BLE “skimmer suspect” results as **serial-module name
candidates**, with the matching name and signal strength. Names such as HC-05
are common in ordinary electronics and do not establish that a payment skimmer
is present. The ESP32-C3 supports Bluetooth LE only, so it cannot see
Classic-Bluetooth-only skimmers. An empty report cannot clear a location. See
[Espressif's C3 BLE overview](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/ble/overview.html)
and [SparkFun's original skimmer-scanner research](https://github.com/sparkfunX/Skimmer_Scanner).

Wi-Fi and BLE share the C3's radio. Sequential scans avoid competing for it,
and the report reuses upstream's bounded result stores. Stop BLE scans before
reporting so the results are stable. The retained terminal output can be saved
through the existing WebUI `/api/logs` path or USB. This first version has no
automatic background alert and no dedicated touch view.
