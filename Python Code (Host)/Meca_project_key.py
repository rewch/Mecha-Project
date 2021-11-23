import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt
import time
import json
import random
import ssl
import requests

port = 1883 # default port
Server_ip = "broker.netpie.io" 

Subscribe_Topic0 = "@msg/esp32/reset_state"
Subscribe_Topic1 = "@msg/esp32/keyboard_password"
Subscribe_Topic2 = "@msg/esp32/encoder_password"
Subscribe_Topic3 = "@msg/esp32/line_request_password"
Subscribe_Topic4 = "@msg/esp32/RFID_password"
Subscribe_Topic5 = "@msg/esp32/line_verified"
Publish_Topic = "@msg/key/verified"
Publish_Topic2 = "@shadow/data/update"

Client_ID = "2264c4c2-7aa0-4d2f-be25-e8dba32d465a"
Token = "HYEuHte8aqrN7voMoLafvpcUmK35M2te"
Secret = "P$DJSMwwQILFGwwe6eCh_qYa5JcQDQie"

url = "https://notify-api.line.me/api/notify"
token = "4FPMU4mwLOnaCEP7LtfVd4AL4g3d7WBIhIr8wMuh1Nk" 
headers = {'Authorization':'Bearer '+token}

MqttUser_Pass = {"username":Token,"password":Secret}

esp_data = ""
topic = ""

Keyboard_Status = "locked"
Encoder_Status = "locked"
RFID_Status = "locked"
Line_Authen_Status = "locked"
shadow_data = {"Keyboard": Keyboard_Status, "Encoder": Encoder_Status, "RFID" : RFID_Status, "LINE" : Line_Authen_Status}

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(Subscribe_Topic0)
    client.subscribe(Subscribe_Topic1)
    client.subscribe(Subscribe_Topic2)
    client.subscribe(Subscribe_Topic3)
    client.subscribe(Subscribe_Topic4)
    client.subscribe(Subscribe_Topic5)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    try :
        # print(msg.topic+" "+str(msg.payload))
        global esp_data
        esp_data = str(json.loads(msg.payload)) #  json --> dict
        global topic
        topic = str(msg.topic)
    except :
        esp_data = msg.payload.decode('UTF-8') 
        topic = str(msg.topic)

    print(esp_data)
    print(topic)
       
client = mqtt.Client(protocol=mqtt.MQTTv311,client_id=Client_ID, clean_session=True,)
client.on_connect = on_connect
client.on_message = on_message

client.subscribe(Subscribe_Topic0)
client.subscribe(Subscribe_Topic1)
client.subscribe(Subscribe_Topic2)
client.subscribe(Subscribe_Topic3)
client.subscribe(Subscribe_Topic4)
client.subscribe(Subscribe_Topic5)
client.username_pw_set(Token,Secret)
client.connect(Server_ip, port)
client.loop_start()

## initialize password text file in json format ##
# data = {}
# data['password'] = []
# data['password'].append({
#     'keyboard': '111111',
#     'encoder': '111111',
#     'RFID': ''
# })
# print(data["password"][0]["keyboard"])
# with open('password.txt', 'w') as outfile:
#     json.dump(data, outfile)
# data_out=json.dumps(data) # in case encode object to JSON

data_out=json.dumps({"data": shadow_data}) 
client.publish(Publish_Topic2, data_out, retain= True)

while True:
        
        with open('password.txt') as json_file :
            data = json.load(json_file)

        if topic ==  Subscribe_Topic1 :
            if esp_data == str(data["password"][0]["keyboard"]) :
                client.publish(Publish_Topic,"correct", retain = False )
                shadow_data["Keyboard"] = "unlocked"
                data_out=json.dumps({"data": shadow_data}) 
                client.publish(Publish_Topic2, data_out, retain= True)

            else :
                client.publish(Publish_Topic, "incorrect", retain=False)
            topic = ""
            esp_data=""  

        elif topic ==  Subscribe_Topic2 :
            if esp_data == str(data["password"][0]["encoder"]) :
                client.publish(Publish_Topic,"correct", retain = False )
                shadow_data["Encoder"] = "unlocked"
                data_out=json.dumps({"data": shadow_data}) 
                client.publish(Publish_Topic2, data_out, retain= True)

            else :
                client.publish(Publish_Topic, "incorrect", retain=False)
            topic = ""
            esp_data=""  
        
        elif topic ==  Subscribe_Topic3 :
            password = ""
            for i in range(6) :
                password+= str(random.randrange(1,6))
            client.publish(Publish_Topic, password, retain=True)
            msg = {
                "message" : password
            }
            res = requests.post(url,headers=headers, data=msg)
            topic = ""
        
        elif topic ==  Subscribe_Topic4 :
            if esp_data == str(data["password"][0]["RFID"]) :
                client.publish(Publish_Topic,"correct", retain = False )
                shadow_data["RFID"] = "unlocked"
                data_out=json.dumps({"data": shadow_data}) 
                client.publish(Publish_Topic2, data_out, retain= True)
            else :
                client.publish(Publish_Topic, "incorrect", retain=False)
            topic = ""
            esp_data="" 

        elif topic ==  Subscribe_Topic5 :
            if esp_data == "correct" :
                shadow_data["LINE"] = "unlocked"
                data_out=json.dumps({"data": shadow_data}) 
                client.publish(Publish_Topic2, data_out, retain= True)
            topic = ""
            esp_data="" 

        elif topic ==  Subscribe_Topic0 :
            if esp_data == "reset" :
                shadow_data = {"Keyboard": Keyboard_Status, "Encoder": Encoder_Status, "RFID" : RFID_Status, "LINE" : Line_Authen_Status}
                data_out=json.dumps({"data": shadow_data}) 
                client.publish(Publish_Topic2, data_out, retain= True)
            topic = ""
            esp_data="" 

        time.sleep(2)
        