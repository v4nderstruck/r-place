import logging
import asyncio
import time
from websockets import client
from websockets.typing import Data

# Constants
NUM_WORKERS = 2  # Number of workers to create
NUM_CONNECTIONS_PER_WORKER = 100  # Number of connections per worker
DURATION = 60  # Duration of the stress test in seconds
SLEEP_TIME = 0.2  # Sleep time between each message in seconds

# WebSocket server URL
SERVER_URL = "ws://localhost:8081/tile"

# Performance measurement variables
stop = False

msg_dict = []


async def worker(worker_id, msg):
    import random
    import datetime
    start = datetime.datetime.now()
    # 16mb frames
    connection = await client.connect(SERVER_URL, max_size=2**24)

    check = []
    while True:
        if (datetime.datetime.now() - start).total_seconds() * 1000 >= DURATION * 1000:
            await connection.close()
            return
        send, to_check = msg[random.randint(0, len(msg) - 1)]
        print("send", send)
        await connection.send(send)
        check.append(to_check)
        recv_data = await connection.recv()
        if isinstance(recv_data, str):
            print(recv_data)
        elif isinstance(recv_data, bytes):
            # check the data is correct, take x * y as offset and check for 4
            # bytes matching the color
            # for check_data in check:
            #     offset = check_data["x"] * check_data["y"]
            #         
            #     truth = check_data["color"].to_bytes(4, "little")
            #     for i in range(4):
            #         print(f"{recv_data[offset + i]} == {truth[i]}" )
            print("recveid bytes")
            for byte in recv_data:
                if byte != 0:
                    print("seems to be some data")
                    continue
            print("seems to be all 0")

        else:
            print("Unknown data type")

        await asyncio.sleep(SLEEP_TIME)


def random_msg():
    """adds random messages to the msg_dict in JSON {x: int, y: int, color: int} format"""
    import random
    import json
    global msg_dict
    msg_dict = []
    for _i in range(10000):
        x = random.randint(0, 999)
        y = random.randint(0, 999)
        color = random.randint(0, 10000)
        msg_dict.append(
            (json.dumps({"x": x, "y": y,
                         "color": color}),
             {"x": x, "y": y, "color": color}
             )
        )


def coroutine(i, msg):
    asyncio.run(worker(i, msg))


def main():
    global msg_dict
    random_msg()
    coroutine(0, msg_dict)


if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s %(message)s",
        level=logging.INFO,
    )
    main()
