#!/usr/bin/env python3
"""Smoke test for JOIN notifications and registration numerics."""

import socket
import subprocess
import sys
import time


HOST = "127.0.0.1"
PORT = 4999
PASSWORD = "test"
CHANNEL = "#join-test"


def receive(client, timeout=1.0):
    client.settimeout(timeout)
    data = b""
    try:
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            data += chunk
            if b"\r\n" not in data:
                continue
            client.settimeout(0.05)
    except socket.timeout:
        pass
    return data.decode("utf-8", errors="replace")


def send(client, command):
    client.sendall((command + "\r\n").encode("utf-8"))


def register(nickname):
    client = socket.create_connection((HOST, PORT), timeout=2)
    send(client, "PASS " + PASSWORD)
    send(client, "NICK " + nickname)
    send(client, "USER " + nickname + " 0 * :" + nickname)
    data = receive(client)
    if " 001 " not in data:
        raise AssertionError(
            "{} was not registered. Server response: {!r}".format(nickname, data)
        )
    return client


def assert_contains(data, expected, description):
    if expected not in data:
        raise AssertionError(
            "{}\nExpected: {!r}\nReceived: {!r}".format(
                description, expected, data
            )
        )


def main():
    server = subprocess.Popen(
        ["./ft-irc", str(PORT), PASSWORD],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    alice = None
    bob = None
    try:
        time.sleep(0.2)
        alice = register("alice")
        bob = register("bob")

        receive(alice)
        receive(bob)

        send(alice, "JOIN " + CHANNEL)
        alice_join = receive(alice)
        assert_contains(
            alice_join,
            ":alice!alice@localhost JOIN :" + CHANNEL + "\r\n",
            "Alice did not receive her JOIN message",
        )
        assert_contains(alice_join, " 353 alice ", "Alice did not receive 353")
        assert_contains(alice_join, " 366 alice " + CHANNEL + " ", "Alice did not receive 366")

        send(bob, "JOIN " + CHANNEL)
        alice_sees_bob = receive(alice)
        bob_join = receive(bob)

        expected_join = ":bob!bob@localhost JOIN :" + CHANNEL + "\r\n"
        assert_contains(
            alice_sees_bob,
            expected_join,
            "Alice did not see Bob join the channel",
        )
        assert_contains(
            bob_join,
            expected_join,
            "Bob did not receive his JOIN message",
        )
        assert_contains(bob_join, " 353 bob ", "Bob did not receive 353")
        assert_contains(
            bob_join,
            " 366 bob " + CHANNEL + " ",
            "Bob did not receive 366",
        )

        print("PASS: JOIN notifications and 353/366 responses are correct")
        return 0
    except (AssertionError, OSError) as error:
        print("FAIL: {}".format(error), file=sys.stderr)
        return 1
    finally:
        for client in (alice, bob):
            if client is not None:
                client.close()
        server.terminate()
        try:
            server.wait(timeout=2)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()


if __name__ == "__main__":
    sys.exit(main())
