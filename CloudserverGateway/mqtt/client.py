from awsiot import mqtt5_client_builder
from awscrt import mqtt5
import threading
import time

ENDPOINT = "a3go3aob15w1as-ats.iot.eu-west-1.amazonaws.com"
CERT_PATH = "ciaran-keys/e882d03186b8dd480a5d7a14ba7323da7f5e9f3be69a6582e516cd8d7b83e022-certificate.pem.crt"
KEY_PATH = "ciaran-keys/e882d03186b8dd480a5d7a14ba7323da7f5e9f3be69a6582e516cd8d7b83e022-private.pem.key"
CLIENT_ID = "ciaran_subscribe"
TOPIC = "test"
TIMEOUT = 30

connection_success_event = threading.Event()
stopped_event = threading.Event()


# When a message is received
def on_publish_received(publish_packet_data):
    publish_packet = publish_packet_data.publish_packet
    payload = publish_packet.payload.decode("utf-8")
    print(f"\nReceived on '{publish_packet.topic}': {payload}")


# When connection succeeds
def on_lifecycle_connection_success(data):
    print("Connected successfully!")
    connection_success_event.set()


# When client stops
def on_lifecycle_stopped(data):
    print("Client stopped.")
    stopped_event.set()

client = mqtt5_client_builder.mtls_from_path(
    endpoint=ENDPOINT,
    cert_filepath=CERT_PATH,
    pri_key_filepath=KEY_PATH,
    client_id=CLIENT_ID,
    on_publish_received=on_publish_received,
    on_lifecycle_connection_success=on_lifecycle_connection_success,
    on_lifecycle_stopped=on_lifecycle_stopped,
)

print("Starting client...")
client.start()

if not connection_success_event.wait(TIMEOUT):
    raise TimeoutError("Connection timed out")

print(f"Subscribing to topic '{TOPIC}'...")

subscribe_future = client.subscribe(
    subscribe_packet=mqtt5.SubscribePacket(
        subscriptions=[
            mqtt5.Subscription(
                topic_filter=TOPIC,
                qos=mqtt5.QoS.AT_LEAST_ONCE
            )
        ]
    )
)

suback = subscribe_future.result(TIMEOUT)
print(f"Subscribed! Reason codes: {suback.reason_codes}")

print("Waiting for messages... Press Ctrl+C to exit.\n")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nStopping client...")
    client.stop()
    stopped_event.wait(TIMEOUT)
    print("Exited cleanly.")
