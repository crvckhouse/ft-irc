#!/usr/bin/env python3

import os
import select
import socket
import subprocess
import time

PORT = 4998


def stop_server(process, clients):
    for client_socket in clients:
        client_socket.close()
    process.terminate()
    process.wait()


def fail(message, process, clients):
    stop_server(process, clients)
    raise SystemExit("FAIL: " + message)


def read_output(process, output, expected, timeout=2):
    deadline = time.time() + timeout
    while expected not in output and time.time() < deadline:
        ready, _, _ = select.select([process.stdout], [], [], 0.1)
        if ready:
            output += os.read(process.stdout.fileno(), 4096).decode()
    return output


def read_output_count(process, output, expected, count, timeout=2):
    deadline = time.time() + timeout
    while output.count(expected) < count and time.time() < deadline:
        ready, _, _ = select.select([process.stdout], [], [], 0.1)
        if ready:
            output += os.read(process.stdout.fileno(), 4096).decode()
    return output


def send_commands(client_socket, *commands):
    client_socket.sendall(("\r\n".join(commands) + "\r\n").encode())


project_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
process = subprocess.Popen(
    ["stdbuf", "-oL", os.path.join(project_directory, "ft-irc")],
    cwd=project_directory,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
clients = []

try:
    for name in ("alice", "bob"):
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.settimeout(1)
        for attempt in range(20):
            try:
                client_socket.connect(("127.0.0.1", PORT))
                break
            except (ConnectionRefusedError, OSError):
                if attempt == 19:
                    fail("the server did not accept " + name, process, clients)
                time.sleep(0.1)
        clients.append(client_socket)

    alice, bob = clients
    output = ""
    send_commands(alice, "NICK alice", "USER alice 0 * :Alice")
    send_commands(bob, "NICK bob", "USER bob 0 * :Bob")

    send_commands(alice, "JOIN #alpha")
    output = read_output(process, output, "NEW CHANNEL NAME : #alpha")
    if "NEW CHANNEL NAME : #alpha" not in output:
        fail("alice did not create #alpha", process, clients)

    send_commands(bob, "JOIN #alpha")
    output = read_output_count(process, output, "MEMBER ADDED", 2)
    if output.count("MEMBER ADDED") < 2:
        fail("bob did not join #alpha", process, clients)

    send_commands(bob, "JOIN #beta")
    output = read_output(process, output, "NEW CHANNEL NAME : #beta")
    if "NEW CHANNEL NAME : #beta" not in output:
        fail("bob did not create #beta", process, clients)

    send_commands(alice, "PART #alpha")
    output = read_output(process, output, "DELETE MEMBERS")
    if "DELETE MEMBERS" not in output:
        fail("alice did not leave #alpha", process, clients)

    send_commands(bob, "PART #alpha")
    output = read_output(process, output, "CHANNEL ERASED")
    if "CHANNEL ERASED" not in output:
        fail("#alpha was not erased after its last member left", process, clients)

    send_commands(bob, "PART #beta")
    output = read_output_count(process, output, "CHANNEL ERASED", 2)
    if output.count("CHANNEL ERASED") < 2:
        fail("#beta was not erased after its last member left", process, clients)
finally:
    stop_server(process, clients)

print("PASS: two clients joined channels and removed them with PART")
