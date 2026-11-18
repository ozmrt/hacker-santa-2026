# Recon and Fox Hunt on the round ESP32-C3

This port uses GhostESP's existing AP and BLE advertisement scanners and RSSI
trackers. The WebUI adds a **Recon** page with a bounded survey kept in the
connected browser. It does not need an SD card or allocate a second radio
database on the C3.

1. Join the board's GhostNet AP and open `http://192.168.4.1/`.
2. Open **Recon**. Run **Scan Wi-Fi APs**, or start and stop a BLE scan.
   Run one radio at a time. The page may disconnect during scans; reconnect to
   GhostNet if necessary.
3. Press **Collect snapshot** after the scan ends. Repeat scans to update
   first/last seen times and snapshot counts. The client retains at most 128
   distinct radio/address pairs; oldest rows are dropped when full.
4. Press **Download CSV** or **Download JSON** to save the survey on the
   computer or phone that opened the WebUI. The browser's own clock supplies
   timestamps. Closing/reloading the page keeps the survey in that browser's
   local storage; clearing browser data removes it. Export before clearing.

`GET /api/recon` uses the existing WebUI authentication setting. It streams the
board's current, bounded AP and BLE result lists as JSON. It refuses to run
while either scan is active. Raw names are reduced to printable ASCII and JSON
escaped; the page displays them as text. CSV exports prefix spreadsheet
formula-like cells with an apostrophe. The browser download is local to the
client; the firmware does not write a file or transmit to a third party.

The **FOX HUNT** button is available for addresses in the latest snapshot.
It starts the existing RSSI tracker on the device. Tracking can interrupt the
board's Wi-Fi AP, so use the device Terminal for `foxhunt status` and
`foxhunt stop` until the AP returns. From the round touch UI, open an AP,
station or BLE scan result and tap **Fox Hunt** to show the existing RSSI ring.
The Terminal command also accepts `foxhunt ap <BSSID>` or
`foxhunt ble <address>` after a scan.

RSSI is affected by reflections, antenna direction and body shielding. A
stronger reading suggests proximity, but it is not a distance measurement.
BLE private addresses may rotate, so address-based deduplication can count one
physical device more than once. The ESP32-C3 sees only 2.4 GHz Wi-Fi and BLE;
it cannot track 5 GHz APs or Classic Bluetooth devices. No GPS positions are
recorded.

The WebUI's security reminder and XSS example are intentionally inert. The
example tag is rendered as text; it does not execute. The satirical advisory
style borrows the tone of modern vulnerability microsites, with original copy
and clear labels for actual observations.
