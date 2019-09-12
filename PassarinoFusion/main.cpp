#include <iostream>
#include <mosquitto.h>
#include <cstdlib>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <mutex>
#include <math.h>

#define ACCELERATION_TOLL 0.1
#define ACCELERATION 75

using namespace std;

void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    //cout << message->topic << " -> " << (char*) message->payload << std::endl;

    char *msg = (char*) message->payload;
    float vt, sg;
    return;
}


void on_connect_callback(struct mosquitto *mosq, void *userdata, int rc) {
    char topic1[] = "mobility/move";
    mosquitto_subscribe(mosq, NULL, topic1, 1);
    return;
}

int main(int argc, char* argv[]) {
    /* VARIABLES DECLARATION*/
    bool running = true;
    void* pacchetto;
    struct mosquitto* mqtt_client = NULL;
    char mosquitto_broker_address[] = "127.0.0.1";
    int mosquitto_broker_port = 1883;
    int mosquitto_timeout_sleep = 60;


    mosquitto_lib_init(); //Starts mosquitto library
    mqtt_client=mosquitto_new(NULL,running,pacchetto); //create a struct mosquitto* object
    mosquitto_connect_callback_set(mqtt_client,on_connect_callback); //Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
    mosquitto_message_callback_set (mqtt_client, on_message_callback); //Set the message callback.  This is called when a message is received from the broker.
    mosquitto_connect(mqtt_client,mosquitto_broker_address,mosquitto_broker_port,mosquitto_timeout_sleep); // Connect to an MQTT broker.
    mosquitto_loop_start(mqtt_client); // This function call loop() for you in an infinite blocking loop.  It is useful for the case where you only want to run the MQTT client loop in your program.It handles reconnecting in case server connection is lost.  If you call mosquitto_disconnect() in a callback it will return.


    while(1) {



        // Publish wheel and Dynamixel commands
        std::string msg = to_string(0) + " " + to_string(0) + " " + to_string(0) + " " + to_string(0) + " " + to_string(0) + " " + to_string(0);
        mosquitto_publish(mqtt_client, 0, "mobility/motorsspeed", msg.length(), msg.c_str(), 0, 0);


        usleep(100000);
    }

    return 0;
}
