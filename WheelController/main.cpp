#include <iostream>
#include <mosquitto.h>
#include <cstdlib>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <mutex>

#define P_C 0.00001 //0.0001
#define I_C 0.0002
#define D_C 0 1000000000


using namespace std;

float fdb[6] = {0, 0, 0, 0, 0, 0};
float desired[6] = {0, 0, 0, 0, 0, 0};

void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    //cout << message->topic << " -> " << (char*) message->payload << std::endl;

    int index = -1;
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/1") == 0) {
        index = 0;
    }
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/2") == 0) {
        index = 1;
    }
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/3") == 0) {
        index = 2;
    }
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/4") == 0) {
        index = 3;
    }
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/5") == 0) {
        index = 4;
    }
    if(strcmp((char*)message->topic, "mobility/feedback/vesc/6") == 0) {
        index = 5;
    }

    if(index < 6 && index >= 0) {
        char *msg = (char*) message->payload;
        sscanf(msg, "%f", &fdb[index]);
        //cout << index << " " << fdb[index] << endl;
    }

    if(strcmp((char*)message->topic, "mobility/motorsspeed") == 0) {
        char *msg = (char*) message->payload;
        sscanf(msg, "%f %f %f %f %f %f", &desired[0], &desired[1], &desired[2], &desired[3], &desired[4], &desired[5]);
    }

    return;
}


void on_connect_callback(struct mosquitto *mosq, void *userdata, int rc) {
    char topic1[] = "mobility/feedback/vesc/#";
    mosquitto_subscribe(mosq, NULL, topic1, 1);
    char topic2[] = "mobility/motorsspeed"; //defines topic, # is the wildcard topic, and gets datas from all topics except for "$" starting topic
    mosquitto_subscribe(mosq, NULL, topic2, 1); //Subscribe to a topic.
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
    mosquitto_loop_start(mqtt_client); // This function call loop() for you in an infinite blocking loop.  It is useful for the case where you only want to run the MQTT client loop in your program.It handles reconnecting in case server connection is lost.  If you call mosquitto_disconnect() in a callback it will return.

    float p[6] = {0, 0, 0, 0, 0, 0};
    float i[6] = {0, 0, 0, 0, 0, 0};
    float d[6] = {0, 0, 0, 0, 0, 0};

    float old_error[6] = {0, 0, 0, 0, 0, 0};

    while(1) {
        float error[6];
        float pid[6];

        for(int y=0;y<6;y++) {
            error[y] = desired[y] - fdb[y];

            p[y] = error[y] * P_C;
            i[y] += error[y] * 0.1 * I_C;
            d[y] = (error[y]-old_error[y])/0.1 * P_C;
            old_error[y] = error[y];

            pid[y] = p[y]+i[y]+d[y];
            //cout << fdb[y] << endl;

            if(pid[y] > 0.2) {
                pid[y] = 0.2;
            }
            if(pid[y] < -0.2) {
                pid[y] = -0.2;
            }
        }

        std::string msg = to_string(pid[0]) + " " + to_string(pid[1]) + " " +
        to_string(pid[2]) + " " + to_string(pid[3]) + " " + to_string(pid[4])
        + " " + to_string(pid[5]);
        mosquitto_publish(mqtt_client, 0, "mobility/VESC/duty", msg.length(), msg.c_str(), 0, 0);

        // cout << msg << endl;

        usleep(100000);
    }

    return 0;
}
