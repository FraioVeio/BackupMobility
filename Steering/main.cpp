#include <iostream>
#include <mosquitto.h>
#include <cstdlib>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <mutex>


using namespace std;

float vtan_command = 0, sigma_command = 0;
float vtan_desired = 0, sigma_desired = 0;

float acceleration = 1;

// Cancellami appena meloni pusha la funzione completa
void steering_angle (double _sigma, double* alpha_wheels);

void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    cout << message->topic << " -> " << (char*) message->payload << std::endl;

    char *msg = (char*) message->payload;
    sscanf(msg, "%f %f", &vtan_command, &sigma_command);

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
    char mosquitto_broker_address[] = "10.0.0.10";
    int mosquitto_broker_port = 1883;
    int mosquitto_timeout_sleep = 60;

    mosquitto_lib_init(); //Starts mosquitto library
    mqtt_client=mosquitto_new(NULL,running,pacchetto); //create a struct mosquitto* object
    mosquitto_connect_callback_set(mqtt_client,on_connect_callback); //Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
    mosquitto_message_callback_set (mqtt_client, on_message_callback); //Set the message callback.  This is called when a message is received from the broker.
    mosquitto_connect(mqtt_client,mosquitto_broker_address,mosquitto_broker_port,mosquitto_timeout_sleep); // Connect to an MQTT broker.
    mosquitto_loop_start(mqtt_client); // This function call loop() for you in an infinite blocking loop.  It is useful for the case where you only want to run the MQTT client loop in your program.It handles reconnecting in case server connection is lost.  If you call mosquitto_disconnect() in a callback it will return.


    float current_speed = 0;
    float current_sigma = 0;

    while(1) {
        // Behaviour control
        vtan_desired = vtan_command;
        sigma_desired = sigma_command;

        // Acceleration control
        if(current_speed < vtan_desired-0.05) {
            current_speed += acceleration * 0.1;
        }

        if(current_speed > vtan_desired+0.05) {
            current_speed -= acceleration * 0.1;
        }

        // Angle control
        double dynamixel_desired[6];
        steering_angle(current_sigma, dynamixel_desired);



        usleep(100000);
    }

    return 0;
}
