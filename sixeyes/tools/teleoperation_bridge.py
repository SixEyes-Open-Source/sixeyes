#!/usr/bin/env python3
"""
SixEyes Teleoperation Bridge (Phase 3)

Bridges leader_esp32 JOINT_STATE messages to follower_esp32 over serial.
Optionally logs forwarded packets to JSONL for dataset collection.

Protocol:
  Leader -> Bridge:   {"cmd":"JOINT_STATE", ...}\n
  Bridge -> Follower: {"cmd":"JOINT_STATE", ...}\n
Usage:
  python teleoperation_bridge.py --leader-port COM5 --follower-port COM6
  python teleoperation_bridge.py --leader-port /dev/ttyUSB0 --follower-port /dev/ttyUSB1 --log-file teleop.jsonl
"""

import argparse
import json
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, TextIO

import serial


@dataclass
class BridgeStats:
    packets_in: int = 0
    packets_forwarded: int = 0
    packets_dropped: int = 0
    parse_errors: int = 0
    last_seq: Optional[int] = None


class TeleoperationBridge:
    def __init__(
        self,
        leader_port: str,
        follower_port: str,
        baudrate: int = 115200,
        log_file: Optional[str] = None,
    ):
        self.leader_port = leader_port
        self.follower_port = follower_port
        self.baudrate = baudrate
        self.log_file_path = Path(log_file) if log_file else None

        self.leader_serial: Optional[serial.Serial] = None
        self.follower_serial: Optional[serial.Serial] = None
        self.log_handle: Optional[TextIO] = None

        self.running = False
        self.stats = BridgeStats()
        self.lock = threading.Lock()
        self.forward_thread: Optional[threading.Thread] = None

    def connect(self) -> bool:
        try:
            self.leader_serial = serial.Serial(
                port=self.leader_port,
                baudrate=self.baudrate,
                timeout=0.1,
                write_timeout=0.2,
            )
            self.follower_serial = serial.Serial(
                port=self.follower_port,
                baudrate=self.baudrate,
                timeout=0.1,
                write_timeout=0.2,
            )

            if self.log_file_path:
                self.log_file_path.parent.mkdir(parents=True, exist_ok=True)
                self.log_handle = self.log_file_path.open("a", encoding="utf-8")

            print(f"[INFO] Leader serial connected: {self.leader_port} @ {self.baudrate}")
            print(f"[INFO] Follower serial connected: {self.follower_port} @ {self.baudrate}")
            if self.log_file_path:
                print(f"[INFO] Logging forwarded packets to: {self.log_file_path}")
            return True

        except serial.SerialException as error:
            print(f"[ERROR] Serial connect failed: {error}")
            self.disconnect()
            return False

    def disconnect(self) -> None:
        if self.log_handle:
            self.log_handle.close()
            self.log_handle = None

        if self.leader_serial and self.leader_serial.is_open:
            self.leader_serial.close()
        if self.follower_serial and self.follower_serial.is_open:
            self.follower_serial.close()

        self.leader_serial = None
        self.follower_serial = None

    def start(self) -> bool:
        if not self.leader_serial or not self.follower_serial:
            print("[ERROR] Bridge not connected. Call connect() first.")
            return False

        self.running = True
        self.forward_thread = threading.Thread(target=self._forward_loop, daemon=True)
        self.forward_thread.start()
        print("[INFO] Teleoperation bridge started")
        return True

    def stop(self) -> None:
        self.running = False
        if self.forward_thread:
            self.forward_thread.join(timeout=2.0)
        self.disconnect()

    def _forward_loop(self) -> None:
        assert self.leader_serial is not None
        assert self.follower_serial is not None

        while self.running:
            try:
                raw_line = self.leader_serial.readline()
                if not raw_line:
                    continue

                line = raw_line.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                with self.lock:
                    self.stats.packets_in += 1

                payload = self._parse_and_validate(line)
                if payload is None:
                    continue

                encoded = (json.dumps(payload, separators=(",", ":")) + "\n").encode("utf-8")
                self.follower_serial.write(encoded)

                with self.lock:
                    self.stats.packets_forwarded += 1
                    self.stats.last_seq = int(payload.get("seq", self.stats.last_seq or 0))

                if self.log_handle:
                    log_record = {
                        "bridge_ts": int(time.time() * 1000),
                        "payload": payload,
                    }
                    self.log_handle.write(json.dumps(log_record) + "\n")
                    self.log_handle.flush()

            except serial.SerialException as error:
                print(f"[ERROR] Serial forwarding failed: {error}")
                self.running = False
                break
            except Exception as error:
                print(f"[ERROR] Unexpected forwarding error: {error}")
                with self.lock:
                    self.stats.packets_dropped += 1

    def _parse_and_validate(self, line: str) -> Optional[dict]:
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            with self.lock:
                self.stats.parse_errors += 1
                self.stats.packets_dropped += 1
            return None

        if payload.get("cmd") != "JOINT_STATE":
            with self.lock:
                self.stats.packets_dropped += 1
            return None

        joints = payload.get("leader_joints")
        valid_mask = payload.get("valid_mask")

        if not isinstance(joints, list) or not isinstance(valid_mask, list):
            with self.lock:
                self.stats.packets_dropped += 1
            return None

        if len(joints) != 6 or len(valid_mask) != 6:
            with self.lock:
                self.stats.packets_dropped += 1
            return None

        return payload

    def print_stats(self) -> None:
        with self.lock:
            print("\n[STATS] Teleoperation Bridge")
            print(f"  Packets in:        {self.stats.packets_in}")
            print(f"  Packets forwarded: {self.stats.packets_forwarded}")
            print(f"  Packets dropped:   {self.stats.packets_dropped}")
            print(f"  Parse errors:      {self.stats.parse_errors}")
            print(f"  Last sequence:     {self.stats.last_seq}")
            print()


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="SixEyes Teleoperation Bridge")
    parser.add_argument("--leader-port", required=True, help="Leader ESP32 serial port (e.g., COM5)")
    parser.add_argument("--follower-port", required=True, help="Follower ESP32 serial port (e.g., COM6)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--log-file", default=None, help="Optional JSONL output path for forwarded packets")
    parser.add_argument("--stats-interval", type=float, default=5.0, help="Stats print interval in seconds")
    return parser


def main() -> int:
    parser = build_arg_parser()
    args = parser.parse_args()

    bridge = TeleoperationBridge(
        leader_port=args.leader_port,
        follower_port=args.follower_port,
        baudrate=args.baud,
        log_file=args.log_file,
    )

    if not bridge.connect():
        return 1

    if not bridge.start():
        bridge.disconnect()
        return 1

    print("[INFO] Running. Press Ctrl+C to stop.")

    try:
        last_stats = time.time()
        while True:
            time.sleep(0.25)
            if time.time() - last_stats >= args.stats_interval:
                bridge.print_stats()
                last_stats = time.time()
    except KeyboardInterrupt:
        print("\n[INFO] Stopping bridge...")
        bridge.stop()
        print("[INFO] Bridge stopped")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
