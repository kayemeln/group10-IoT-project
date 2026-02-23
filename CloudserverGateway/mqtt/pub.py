import paho.mqtt.publish as publish
MQTT_SERVER = "172.20.10.2"  #Write Server IP Address
MQTT_PATH = "Image"

f=open("IMG_2022.jpg", "rb") #3.7kiB in same folder
fileContent = f.read()
byteArr = bytearray(fileContent)


publish.single(MQTT_PATH, byteArr, hostname=MQTT_SERVER)

# /opt/homebrew/etc/mosquitto/mosquitto.conf