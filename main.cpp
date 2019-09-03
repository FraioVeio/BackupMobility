#include <iostream>
#include <mosquitto.h>
#include <cstdlib>
#include <time.h>
#include <stdlib.h>
#include <fstream>

using namespace std;


void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    cout << message->topic << " -> " << (char*) message->payload << std::endl; //write in the file time topic payload

    return;
}


void on_connect_callback(struct mosquitto *mosq, void *userdata, int rc) {
    char topic[] = "#"; //defines topic, # is the wildcard topic, and gets datas from all topics except for "$" starting topic
    mosquitto_subscribe(mosq, NULL,topic,1); //Subscribe to a topic.
    return;
}


int main(int argc,char* argv[]) {
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
    mosquitto_loop_forever(mqtt_client,1000,10); // This function call loop() for you in an infinite blocking loop.  It is useful for the case where you only want to run the MQTT client loop in your program.It handles reconnecting in case server connection is lost.  If you call mosquitto_disconnect() in a callback it will return.

    return 0;
}
