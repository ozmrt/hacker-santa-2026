# Per My Last Login: offline lab PoC

This board-only captive portal demonstrates two **fixed fixtures**: a PayPal
sandbox scenario and a card test-token scenario. It does not connect to PayPal
or a payment processor, authenticate an account, request card numbers, or accept
free-text credentials. Its wry confirmation appears after a fixture submission.
The resulting JSON Lines file includes the plaintext **synthetic** identity,
password and test token associated with each fixture. They are fixed locally
in the collector, not received from a visitor. The file is a PoC of the portal
and USB evidence path, not proof of a real payment or credential capture.

The page is compiled into flash and adds no retained HTML buffer. The form
accepts only `fixture=paypal-sandbox` or `fixture=card-test-token`; other fields
are rejected. The upstream keystroke logger and general terminal/WebUI history
are bypassed for this mode. Events go to the attached USB console and are not
stored on the ESP32. The host collector creates a new file with permissions
`0600`, writes each event promptly, and never records raw serial output.

After flashing the matching firmware, run the collector **before** connecting
the test client. Use the installed ESP-IDF Python environment, which has
`pyserial`:

```sh
.tools/idf-tools/python_env/idf6.1_py3.15_env/bin/python \
  lab_poc.py --output build/lab-poc.jsonl
```

The collector opens USB, waits for board startup, starts the open AP
`LAB-PerMyLastLogin`, listens for 120 seconds, then stops the portal. Use
`--duration 0` to listen until Ctrl-C. `--ssid` accepts a 5–31 character name
starting `LAB-`; `--port` selects a different USB device. The output path must
not exist already. A macOS serial open may reset the board, which is why the
collector starts the portal itself.

On a test phone/computer, join the lab AP and open `http://192.168.4.1/login`.
Submit each fixed fixture. The page confirms receipt. The host artifact records
UTC host time, board uptime, fixture ID, client IPv4 bytes, free internal heap,
the built application's SHA-256 and the patch SHA-256. A browser screenshot can
be saved alongside the JSONL file to show the visible page; it is a separate
client-side artifact.

The lab AP is open and local. Do not enter real account or card details. A
fixture submission only establishes that the form reached the ESP32 and the
event reached USB. Runtime memory headroom and portal reliability still need
physical testing on this C3 board.
