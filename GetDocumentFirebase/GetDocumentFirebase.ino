#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>
#include <ArduinoJson.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Dialog 4G 038"
#define WIFI_PASSWORD "576fAbd1"

/* 2. Define the API Key */
#define API_KEY "AIzaSyBtcbT4-0wssbZno2jEQYTTZPIlX4jSwuI"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "new-project-21c52"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "hashan@gmail.com"
#define USER_PASSWORD "19960205"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;

unsigned long dataMillis = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
    // https://github.com/mobizt/Firebase-ESP-Client#about-firebasedata-object
    // fbdo.keepAlive(5, 5, 1);
}

void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.

    if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))
    {
        dataMillis = millis();

        String documentPath = "zone/device";

        // If the document path contains space e.g. "a b c/d e f"
        // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"

        Serial.print("Get a document... ");

        if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), "")){

            Serial.println(fbdo.payload().c_str());
            // Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

            // Parse the JSON data
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, fbdo.payload().c_str());

            // Extract the specific fields
            const char* status = doc["fields"]["status"]["stringValue"];
            int pyrethroids = doc["fields"]["pyrethroids"]["integerValue"];
            int water = doc["fields"]["water"]["integerValue"];
            int bordeaux_mixture = doc["fields"]["bordeaux_mixture"]["integerValue"];
            int chlorothalonil = doc["fields"]["chlorothalonil"]["integerValue"];

            int flowRatePyrethroids = doc["fields"]["flowRatePyrethroids"]["integerValue"];
            int flowRateWater = doc["fields"]["flowRateWater"]["integerValue"];
            int flowRateBordeaux_mixture = doc["fields"]["flowRateBordeaux_mixture"]["integerValue"];
            int flowRateChlorothalonil = doc["fields"]["flowRateChlorothalonil"]["integerValue"];


            Serial.println(water);
            Serial.println(pyrethroids);
            Serial.println(status);

            // Call the function to update the status field
            updateStatus(water, pyrethroids, bordeaux_mixture, chlorothalonil, flowRatePyrethroids, flowRateWater, flowRateBordeaux_mixture, flowRateChlorothalonil);
        }

        else{
          Serial.println(fbdo.errorReason());
        }
          
    }
}

// Function to update the status field in the Firestore document
void updateStatus(int water, int pyrethroids, int bordeaux_mixture, int chlorothalonil, int flowRatePyrethroids, int flowRateWater, int flowRateBordeaux_mixture, int flowRateChlorothalonil) {

    // The dynamic array of write object fb_esp_firestore_document_write_t.
    std::vector<struct fb_esp_firestore_document_write_t> writes;

    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Edit_Parse/Create_Edit_Parse.ino
    FirebaseJson content;
    String documentPath = "zone2/device";

    // A write object that will be written to the document.
    struct fb_esp_firestore_document_write_t update_write;

    update_write.type = fb_esp_firestore_document_write_type_update;

    // Set the field to update (status) without affecting other fields
    content.set("fields/status/stringValue", "done");
    content.set("fields/water/integerValue", water);
    content.set("fields/pyrethroids/integerValue", pyrethroids);
    content.set("fields/bordeaux_mixture/integerValue", bordeaux_mixture);
    content.set("fields/chlorothalonil/integerValue", chlorothalonil);

    content.set("fields/flowRatePyrethroids/integerValue", flowRatePyrethroids);
    content.set("fields/flowRateWater/integerValue", flowRateWater);
    content.set("fields/flowRateBordeaux_mixture/integerValue", flowRateBordeaux_mixture);
    content.set("fields/flowRateChlorothalonil/integerValue", flowRateChlorothalonil);

    // Set the update document content
    update_write.update_document_content = content.raw();
    // Set the update document path
    update_write.update_document_path = documentPath.c_str();
    // Add a write object to a write array.
    writes.push_back(update_write);

    Serial.println("Update status... ");

    if (Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());
}
