/*
 * Code pour puce ESP8266
 * - Connexion à un réseau Wi-Fi domestique
 * - LED stable: pas de problème, clignotante: en attente, SOS: aïe
 * - deep sleep pour économiser l'énergie
 * A faire: rotation lente pour la longue aiguille
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// nos codes pour le wi-fi
const char* ssid = "LE-WIFI";
const char* password = "LE-MDP";

// backup wi-fi
const char* ssidb = "LE-WIFI-2";
const char* passwordb = "LE-MDP-2";

// notre broker MQTT
const char* mqtt_server = "broker.mqttdashboard.com";

// LED externe et servomoteur
int ledPin = 13;
int WiFiattempts = 0;
int MQTTattempts = 0;
Servo myservo;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  digitalWrite(ledPin, HIGH);
  delay(100);

  Serial.print("On se connecte a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(ledPin, LOW);
    delay(250);
    digitalWrite(ledPin, HIGH);
    delay(250);
    Serial.print(".");
    WiFiattempts += 1;

    // TODO console: tester si ca marche vraiment comme ca devrait
    if( WiFiattempts == 10 ){
      Serial.println("backup wifi");
      WiFi.begin(ssidb, passwordb);
    }
  }
  randomSeed(micros());
  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Commande du broker :");
  digitalWrite(ledPin, LOW);

  for(int i=0;i<length;i++)
  {
    // Serial.println(payload[i]);
    if((int)payload[i]>194||(int)payload[i]<0)
    break;

    Serial.print((int)payload[i]);
    Serial.println();

    // TODO On bouge le servo progressivement
    // en utilisant d'abord servo.read()
    int currentPos = myservo.read();
    Serial.print("Current pos = ");
    Serial.print((int) currentPos);
    Serial.println();

    myservo.write((int)payload[i]);
    break;
  }

  delay(250);
  digitalWrite(ledPin, HIGH);

  // on dort une minute
  Serial.println("Je pars en deep sommeil");
  ESP.deepSleep(10e6); // (10 secs pour les tests)

}//end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Connection MQTT...");
    // ID random
    String clientId = "BIEG-";
    clientId += String(random(0xffff), HEX);

    // On tente la connexion
    // NB Ajout mdp?
    if (client.connect(clientId.c_str()))
    {
      Serial.println("Connexion faite");

      // Souscription à la commande
      client.subscribe("bieg001");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("nouvel essai dans 5 sec");
      MQTTattempts += 1;

      // Ici, s'arreter apres 2 essai (-> deesleep?)
      // (de toute facon: deep sleep puis le machin refait tout le processus)

      if(MQTTattempts > 2){
        Serial.println("Deep sleep apres deux MQQTattempts ratees");
        ESP.deepSleep(60e6); // 60e6 = 1 minute
      }

      delay(5000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  myservo.attach(2);  // NodeMCU: GIO2 = D4
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
