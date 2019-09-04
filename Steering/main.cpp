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

void steering_angle(double _sigma, double *alpha_deg){

    //Variable Declaration
    double sigma = _sigma; // streering angle in deg
    double theta = 0; // steering angle in rad

    theta = 0.017453292519943295 * sigma; // theta = deg2rad(sigma)

    //curvature radius computation
    if (theta == 0.0) {
      r = 5.0E+8; //set to Inf
    } else {
      r = 250.0 / sin(theta); //250 is a geometry driven parameter
    }

    alpha_deg[0] = atan(-512.0 / (r * cos(theta) + 250.0)) * 57.295779513082323;
    alpha_deg[1] = atan(-512.0 / (r * cos(theta) - 250.0)) * 57.295779513082323;
    alpha_deg[2] = atan(612.0 / (r * cos(theta) + 250.0)) * 57.295779513082323;
    alpha_deg[3] = atan(612.0 / (r * cos(theta) - 250.0)) * 57.295779513082323;
}

void wheels_speed(double _vtan, double _sigma, double *_w) {

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


    float current_vtan = 0;
    float current_sigma = 0;

    // Behaviour variables
    float vtan_command_old = 0
    float sigma_command_old = 0;

    while(1) {
        // Behaviour control
        vtan_desired = vtan_command;
        sigma_desired = sigma_command;

        // Acceleration control
        if(current_vtan < vtan_desired-0.05) {
            current_vtan += acceleration * 0.1;
        }

        if(current_vtan > vtan_desired+0.05) {
            current_vtan -= acceleration * 0.1;
        }

        // Final commands computation
        double dynamixel_angle[4];
        double wheels_w[6];
        steering_angle(current_sigma, dynamixel_angle);
        wheels_speed(current_sigma, current_vtan, wheels_w);


        // Publish wheel and Dynamixel commands
        std::string msg = wheels_w[0] + " " + wheels_w[1] + " " + wheels_w[2] + " " + wheels_w[3] + " " + wheels_w[4] + " " + wheels_w[5];
        mosquitto_publish(mqtt_client, 0, "mobility/motorsspeed", msg.length(), msg.c_str(), 0, 0);

        msg = dynamixel_angle[0] + " " + dynamixel_angle[1] + " " + dynamixel_angle[2] + " " + dynamixel_angle[3];
        mosquitto_publish(mqtt_client, 0, "mobility/dynamixel", msg.length(), msg.c_str(), 0, 0);


        usleep(100000);
    }

    return 0;
}
