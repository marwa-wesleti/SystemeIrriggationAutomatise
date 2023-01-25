//dht config
#include <DHT.h>
#define brocheDeBranchementDHT 22    // La ligne de communication du DHT11 sera donc branchée sur la pin 22 de l'ESP32
#define typeDeDHT DHT11             // Ici, le type de DHT utilisé est un DHT11 

// Instanciation de la librairie DHT
DHT dht(brocheDeBranchementDHT, typeDeDHT);
//soil config 
const int sensor_pin = A0;  // Soil moisture sensor O/P pin //
int humiditeSol;
int sensor_analog;
String etatLumiere;


//ldr config
#include <math.h>
#define resistanceNominal 10000  // Mettre ici la "vraie" valeur (mesurée) de la résistance fixe de 10 kohms que vous allez utiliser
#define pinAnalogique  A6    // L'entrée A3 de l'esp32 sera utilisée pour lire la tension en point milieu du pont diviseur
#define tensionAlimentation  5  // Ici, on utilise un esp32, fonctionnant sous 5 volts
int valeurAnalogique;          // Contiendra la valeur lue sur l'entrée analogique ; pour rappel, il s'agit d'une valeur 12 bits (0..4095)
float tensionAnalogique; // Contiendra la valeur de calcul de tension, exprimée en volt, à partir de la "valeurDeLentreeAnalogique"
int resistanceLDR;              // Contiendra la valeur ohmique calculée de la photorésistance


// relais config
#define pinPompe 23 


//partie firebase
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
//#include <Firebase_ESP_Client.h>
#include <FirebaseESP32.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Microsoft WiFi@TUNISIA MESRS 4C"
#define WIFI_PASSWORD "TUNISIA2018"
#define USER_EMAIL "marwa.wesleti007@gmail.com"
#define USER_PASSWORD "marwa3121"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDZJ2ypQ-LGyRqfXEopOhSS6r_bsQGfHe0"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://systemearrosageautomatis-60572-default-rtdb.firebaseio.com/" 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;

                                    



void setup() {
Serial.begin(115200); // Define baud rate for serial communication //
  // Initialisation du DHT11;
  dht.begin();
  
// partie relais
pinMode(pinPompe,OUTPUT);
digitalWrite(pinPompe,LOW);

//partie Firebase
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
     
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
   
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

 

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
}

void loop() {
//partie soil moinsture
  
  int sensor_analog;
  sensor_analog = analogRead(sensor_pin);
  humiditeSol =map(sensor_analog,0,4096,0,100);
  Serial.print("HumiditeSol = ");
  Serial.print(humiditeSol);
  Serial.println("%");
  
// partie DHT11
  // Lecture des données
  int tauxHumidite = dht.readHumidity();              // Lecture du taux d'humidité (en %)
  int temperatureEnCelsius = dht.readTemperature();   // Lecture de la température, exprimée en degrés Celsius
  // Vérification si données bien reçues
  if (isnan(tauxHumidite) || isnan(temperatureEnCelsius)) {
    Serial.println("Aucune valeur retournée par le DHT11. Est-il bien branché ?");
    delay(2000);
    return;         // Si aucune valeur n'a été reçue par l'Arduino, on attend 2 secondes, puis on redémarre la fonction loop()
  }
   // Affichage des valeurs
  Serial.print("HumiditeClimat = "); Serial.print(tauxHumidite); Serial.println("%");
  Serial.print("TemperatureClimat = "); Serial.print(temperatureEnCelsius); Serial.println("C");
  // Temporisation de 2 secondes (pour rappel : il ne faut pas essayer de faire plus d'1 lecture toutes les 2 secondes, avec le DHT11, selon le fabricant)
 
// partie LDR
  // Lecture de l'entrée analogique (pour rappel, cela retourne une valeur sur 10 bits, comprise entre 0 et 1023)
  valeurAnalogique = analogRead(pinAnalogique);
  // Détermination de la tension présente sur l'entrée analogique
  tensionAnalogique = float(tensionAlimentation * valeurAnalogique) / 4095;
      // diviseur de tension
      //   → LDR = R*(Vcc – Vs) / Vs
      //   → d'où valLDR = R * (tensionAlimArduino – tensionEntreeAnalogique) / tensionEntreeAnalogique
  resistanceLDR = resistanceNominal * (tensionAlimentation- tensionAnalogique) / tensionAnalogique;
  Serial.print("Resistance LDR : ");
  Serial.print(resistanceLDR);
  Serial.println(" ohms");
  if  (resistanceLDR<1000){
    etatLumiere="jour";
  }
   if(resistanceLDR>=1000 && resistanceLDR<60000){
     etatLumiere="lumiére ambiante";
  }
  if(resistanceLDR>=60000){
    etatLumiere="nuit";
  }
  
Serial.println();
// partie pompe
if(humiditeSol>50){
  if(temperatureEnCelsius>=2 && temperatureEnCelsius<10){
   if  (etatLumiere=="jour"){
    Serial.println("Arosser 5s");
    digitalWrite(pinPompe,HIGH);
    delay(5000); //5s d'arrosage
    digitalWrite(pinPompe,LOW);
   }
  }
   if(temperatureEnCelsius>=10 && temperatureEnCelsius<25){
     Serial.println("Arosser 10s");
    digitalWrite(pinPompe,HIGH);
    delay(10000); //10s d'arrosage
    digitalWrite(pinPompe,LOW);
  }
   if(temperatureEnCelsius>=25){
    if  (etatLumiere=="nuit"){
    Serial.println("Arosser 15s");
    digitalWrite(pinPompe,HIGH);
    delay(15000); //15s d'arrosage
    digitalWrite(pinPompe,LOW);
    }
  }
  
}
else digitalWrite(pinPompe,LOW);
delay(1000);

// afficher les valeur dans firebase 
if (Firebase.ready()  &&  (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    //afficher valeur de la temperature
    if (Firebase.RTDB.setInt(&fbdo, "capteur/TemperatureClimat", temperatureEnCelsius)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println(temperatureEnCelsius);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
     //afficher valeur de la Humidite du climat
     if (Firebase.RTDB.setInt(&fbdo, "capteur/HumiditeClimat", tauxHumidite)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println(tauxHumidite);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //afficher valeur de l'intensite du lumiere
     if (Firebase.RTDB.setString(&fbdo, "capteur/LDR", etatLumiere)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println(resistanceLDR);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //afficher valeur de l'humidite du sol
     if (Firebase.RTDB.setInt(&fbdo, "capteur/HumiditeSol", humiditeSol)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println(humiditeSol);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
}
else{
Serial.println("erreur de connexion");
}
    
    

}
