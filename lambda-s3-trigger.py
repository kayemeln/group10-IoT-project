import json
import urllib.parse
import os
import boto3
from botocore.exceptions import ClientError

print("Loading function")

s3 = boto3.client("s3")
rekognition = boto3.client("rekognition")
iot = boto3.client("iot")
endpoint = iot.describe_endpoint(endpointType="iot:Data-ATS")["endpointAddress"]  # recommended endpoint type :contentReference[oaicite:4]{index=4}
iot_data = boto3.client("iot-data", endpoint_url=f"https://{endpoint}")

# Put your collection name in an env var, or hardcode it
COLLECTION_ID = os.environ.get("REKOGNITION_COLLECTION_ID", "my-face-collection")

ENROLL_PREFIX = "public/enroll/"
POLL_PREFIX = "public/poll/"

def publish_command(topic: str, payload: dict):
    iot_data.publish(
        topic=topic,
        qos=1,
        payload=json.dumps(payload).encode("utf-8"),
    )

def ensure_collection(collection_id: str) -> None:
    """
    Create the Rekognition collection if it doesn't exist.
    """
    try:
        rekognition.describe_collection(CollectionId=collection_id)
        # Exists
        return
    except ClientError as e:
        code = e.response["Error"].get("Code", "")
        if code == "ResourceNotFoundException":
            rekognition.create_collection(CollectionId=collection_id)
            print(f"Created collection: {collection_id}")
            return
        raise


def enroll_face(bucket: str, key: str) -> dict:
    """
    Index face(s) from the S3 image into the collection.
    """
    ensure_collection(COLLECTION_ID)

    # ExternalImageId is how you later know "who" this face belongs to.
    # Here we use filename without extension. You can replace with your user id logic.
    filename = key.split("/")[-1]
    external_image_id = os.path.splitext(filename)[0] if "os" in globals() else filename.split(".")[0]

    # If you only ever want 1 face per enroll image, keep MaxFaces=1.
    # If you want to index multiple faces, remove MaxFaces.
    resp = rekognition.index_faces(
        CollectionId=COLLECTION_ID,
        Image={"S3Object": {"Bucket": bucket, "Name": key}},
        ExternalImageId=external_image_id[:255],
        DetectionAttributes=[],
        MaxFaces=1,
        QualityFilter="AUTO",
    )

    face_records = resp.get("FaceRecords", [])
    unindexed = resp.get("UnindexedFaces", [])

    print(f"Indexed faces: {len(face_records)}; Unindexed: {len(unindexed)}")
    if face_records:
        fr = face_records[0]
        face_id = fr["Face"]["FaceId"]
        image_id = fr["Face"]["ImageId"]
        confidence = fr["Face"].get("Confidence")
        print(f"Enrolled FaceId={face_id}, ImageId={image_id}, Confidence={confidence}")

    if unindexed:
        # Helpful for debugging why a face wasn't accepted (blur, low quality, etc.)
        reasons = [u.get("Reasons", []) for u in unindexed]
        print(f"Unindexed reasons: {reasons}")

    return {
        "action": "enroll",
        "bucket": bucket,
        "key": key,
        "collection": COLLECTION_ID,
        "indexed": len(face_records),
        "unindexed": len(unindexed),
        "rekognition_response": resp,
    }


def poll_face(bucket: str, key: str) -> dict:
    """
    Search for the best matching face in the collection for the S3 image.
    """
    # If you want poll to create the collection if missing, call ensure_collection here too.
    # Often you *don't* want that; you'd rather return "no collection / no matches".
    try:
        rekognition.describe_collection(CollectionId=COLLECTION_ID)
    except ClientError as e:
        code = e.response["Error"].get("Code", "")
        if code == "ResourceNotFoundException":
            print(f"Collection does not exist: {COLLECTION_ID}")
            return {
                "action": "poll",
                "bucket": bucket,
                "key": key,
                "collection": COLLECTION_ID,
                "match": None,
                "reason": "collection_not_found",
            }
        raise

    resp = rekognition.search_faces_by_image(
        CollectionId=COLLECTION_ID,
        Image={"S3Object": {"Bucket": bucket, "Name": key}},
        FaceMatchThreshold=90,  # adjust to taste
        MaxFaces=5,
        QualityFilter="AUTO",
    )

    matches = resp.get("FaceMatches", [])
    if not matches:
        print("No face matches found.")

        publish_command(
            topic="test",
            payload={
                "event": "no_match",
                "bucket": bucket,
                "key": key,
            },
        )

        return {
            "action": "poll",
            "bucket": bucket,
            "key": key,
            "collection": COLLECTION_ID,
            "match": None,
            "rekognition_response": resp,
        }

    best = matches[0]
    face = best["Face"]
    similarity = best["Similarity"]

    print(
        f"Best match FaceId={face['FaceId']}, ExternalImageId={face.get('ExternalImageId')}, Similarity={similarity}"
    )
    publish_command(
        topic="test",
        payload={
            "event": "match",
            "bucket": bucket,
            "key": key,
        },
    )

    return {
        "action": "poll",
        "bucket": bucket,
        "key": key,
        "collection": COLLECTION_ID,
        "match": {
            "FaceId": face["FaceId"],
            "ExternalImageId": face.get("ExternalImageId"),
            "Similarity": similarity,
            "Confidence": face.get("Confidence"),
        },
        "rekognition_response": resp,
    }


def lambda_handler(event, context):
    bucket = event["Records"][0]["s3"]["bucket"]["name"]
    key = urllib.parse.unquote_plus(
        event["Records"][0]["s3"]["object"]["key"], encoding="utf-8"
    )

    print("key name:", key)

    # Optional: quick content-type guard (keeps logs clean if non-images get uploaded)
    # You can skip this if your prefixes only get images.
    head = s3.head_object(Bucket=bucket, Key=key)
    content_type = head.get("ContentType", "")
    print("CONTENT TYPE:", content_type)

    if not content_type.startswith("image/"):
        return {"ignored": True, "reason": "not_an_image", "contentType": content_type}

    if key.startswith(ENROLL_PREFIX):
        return enroll_face(bucket, key)

    if key.startswith(POLL_PREFIX):
        return poll_face(bucket, key)

    return {"ignored": True, "reason": "prefix_not_handled", "bucket": bucket, "key": key}
