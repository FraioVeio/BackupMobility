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

float vtan_command = 0, sigma_command = 0;
float vtan_desired = 0, sigma_desired = 0;

void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    cout << message->topic << " -> " << (char*) message->payload << std::endl;

    char *msg = (char*) message->payload;
    float vt, sg;
    sscanf(msg, "%f %f", &vt, &sg);
    vt *= 1000;


    // Gestione angoli grossi
    while(sg > 180) {
        sg -= 360;
    }
    while(sg < -180) {
        sg += 360;
    }

    if(vt < 0)
        vt = -vt;

    if(sg < -90) {
        sg = -180 - sg;
        vt = -vt;
    }

    if(sg > 90) {
        sg = 180 - sg;
        vt = -vt;
    }

    vtan_command = vt;
    sigma_command = sg;
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
    double r; // steering radius

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
    if(_sigma != 0) {
        const double wcent = 70; // distanza coppia ruote centrali
        const double wext = 50; // distanza coppia ruote davanti e dietro
        const double wdist = 60; // distanza ruote stesso lato

        double r; // steering radius

        double theta = 0.017453292519943295 * _sigma;

        //curvature radius computation
        r = sqrt(wdist*wdist/(sin(theta)*sin(theta)) - wdist*wdist) * (tan(theta) > 0 ? 1 : -1);

        // Wheel radius
        double radius[6];
        radius[2] = r + wcent/2 * (tan(theta) > 0 ? 1 : -1);
        radius[3] = r - wcent/2 * (tan(theta) > 0 ? 1 : -1);
        radius[0] = sqrt((r+wext/2)*(r+wext/2) + wdist*wdist) * (tan(theta) > 0 ? 1 : -1);
        radius[1] = sqrt((r-wext/2)*(r-wext/2) + wdist*wdist) * (tan(theta) > 0 ? 1 : -1);
        radius[4] = sqrt((r+wext/2)*(r+wext/2) + wdist*wdist) * (tan(theta) > 0 ? 1 : -1);
        radius[5] = sqrt((r-wext/2)*(r-wext/2) + wdist*wdist) * (tan(theta) > 0 ? 1 : -1);
        double radiusvtan = sqrt(r*r + wdist*wdist);

        // Wheels speed
        for(int i=0;i<6;i++) {
            _w[i] = _vtan * radius[i] / radiusvtan * (tan(theta) > 0 ? 1 : -1);
        }
    } else {
        for(int i=0;i<6;i++) {
            _w[i] = _vtan;
        }
    }
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


    float current_vtan = 0;
    float current_sigma = 0;

    // Behaviour variables
    float vtan_command_old = 0;
    float sigma_command_old = 0;
    int stopandgostatus = 0;
    int stopandgocyclecount = 0;



    while(1) {
        // Behaviour control
        bool defaultbehaviour = true;
        if(sigma_command != sigma_command_old) {    // Se c'è stato un cambio di angolo
            if(sigma_command == 45)
                sigma_command = 44.9;

            if((sigma_command_old < 45 && sigma_command > 45) || (sigma_command_old > 45 && sigma_command < 45)) {
                // IMPOSTA LA FLAG CHE FA QUESTO:

                // Velocità a zero e angolo lascia costante
                // DOPO essersi fermati allora cambia l'angolo a quello desiderato
                // Velocità a Velocità desiderata

                stopandgostatus = 1;
            }
        }

        if(stopandgostatus > 0) {
            defaultbehaviour = false;
            if(stopandgostatus == 1) {  // Fermati
                vtan_desired = 0;
                if(current_vtan > -ACCELERATION_TOLL && current_vtan < ACCELERATION_TOLL) { // Quando si è fermato
                    stopandgostatus = 2;
                    stopandgocyclecount = 0;
                }
            }

            if(stopandgostatus == 2) {   // DOPO essersi fermati allora cambia l'angolo a quello desiderato e aspetta
                sigma_desired = sigma_command;

                if(stopandgocyclecount >= 4 * 10) { // aspetta 4 secondi
                    stopandgostatus = 3;
                }

                stopandgocyclecount ++;
            }

            if(stopandgostatus == 3) {   // Velocità a Velocità desiderata
                vtan_desired = vtan_command;
                stopandgostatus = 0;    // esci da stopandgo
            }
        }

        if(defaultbehaviour) {
            vtan_desired = vtan_command;
            sigma_desired = sigma_command;
        }

        vtan_command_old = vtan_command;
        sigma_command_old = sigma_command;

        /***********************/

        // cout << current_vtan << " " << current_sigma << endl;

        // Acceleration control
        if(current_vtan < vtan_desired-ACCELERATION_TOLL) {
            current_vtan += ACCELERATION * 0.1;
        }

        if(current_vtan > vtan_desired+ACCELERATION_TOLL) {
            current_vtan -= ACCELERATION * 0.1;
        }

        current_sigma = sigma_desired;

        // Final commands computation
        double dynamixel_angle[4];
        double wheels_w[6];
        steering_angle(current_sigma, dynamixel_angle);
        wheels_speed(current_vtan, current_sigma, wheels_w);


        // Publish wheel and Dynamixel commands
        std::string msg = to_string(wheels_w[0]) + " " + to_string(wheels_w[1]) + " " + to_string(wheels_w[2]) + " " + to_string(wheels_w[3]) + " " + to_string(wheels_w[4]) + " " + to_string(wheels_w[5]);
        mosquitto_publish(mqtt_client, 0, "mobility/motorsspeed", msg.length(), msg.c_str(), 0, 0);

        msg = to_string(dynamixel_angle[0]) + " " + to_string(dynamixel_angle[1]) + " " + to_string(dynamixel_angle[2]) + " " + to_string(dynamixel_angle[3]);
        mosquitto_publish(mqtt_client, 0, "mobility/dynamixel", msg.length(), msg.c_str(), 0, 0);


        usleep(100000);
    }

    return 0;
}
