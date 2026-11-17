#!/usr/bin/env python3
"""Check that only bounded fixed-fixture serial frames become evidence."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from lab_poc import parse_frame  # noqa: E402


event = parse_frame(b"@LABPOC1\tpaypal-sandbox\t1234\t01020304\t20480\r\n")
assert event is not None
assert event["fixture"] == "paypal-sandbox"
assert event["free_internal_heap_bytes"] == 20480
assert parse_frame(b"@LABPOC1\tother\t1234\t01020304\t20480\n") is None
assert parse_frame(b"@LABPOC1\tcard-test-token\t1\t01020304\t2\tsecret\n") is None
assert parse_frame(b"@LABPOC1\tcard-test-token\t1\t01020304\n") is None
print("Lab PoC host frame tests passed")
