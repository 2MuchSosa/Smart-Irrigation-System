// MQTT Library (includes framework only)
//Aisosa Okunbor and Caleb Smith Team 12

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: -
// Target uC:       -
// System Clock:    -

// Hardware configuration:
// -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "timer.h"
#include "irriSystem.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------
#define MAX_PACKET_SIZE 1518
// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt()
{
    if (getTcpState(0) == TCP_ESTABLISHED)
    {
        uint8_t mqttData[100];
        uint16_t mqttDataSize = 0;
        // MQTT CONNECT PACKET
        mqttData[mqttDataSize++] = 0x10; // connect
        mqttData[mqttDataSize++] = 16; // remaining length FILL IN LATER

        mqttData[mqttDataSize++] = 0x00; // protocol name length MSB
        mqttData[mqttDataSize++] = 0x04; // protocol name length LSB
        mqttData[mqttDataSize++] = 'M';  // protocol name
        mqttData[mqttDataSize++] = 'Q';  // protocol name
        mqttData[mqttDataSize++] = 'T';  // protocol name
        mqttData[mqttDataSize++] = 'T';  // protocol name
        mqttData[mqttDataSize++] = 0x04; // protocol level
        mqttData[mqttDataSize++] = 0x00; // connect flags
        mqttData[mqttDataSize++] = 0x00; // keep alive MSB
        mqttData[mqttDataSize++] = 0x3C; // keep alive LSB 60 seconds

        mqttData[mqttDataSize++] = 0x00; // client id length MSB
        mqttData[mqttDataSize++] = 0x04; // client id length LSB
        mqttData[mqttDataSize++] = 'S';  // client id
        mqttData[mqttDataSize++] = 'O';  // client id
        mqttData[mqttDataSize++] = 'S';  // client id
        mqttData[mqttDataSize++] = 'A';  // client id
        // make ether
        uint8_t buffer[MAX_PACKET_SIZE];
        etherHeader *data = (etherHeader*) buffer;
        sendTcpMessage(data, getsocket(0), PSH | ACK, mqttData, mqttDataSize);
    }
}

void disconnectMqtt()
{
    if (getTcpState(0) == TCP_ESTABLISHED)
    {
        uint8_t mqttData[2];
        // MQTT disconnect packet
        mqttData[0] = 0xE0; // disconnect
        mqttData[1] = 0x00; // remaining length


        uint8_t buffer[MAX_PACKET_SIZE];
        etherHeader *data = (etherHeader*) buffer;
        socket *s = getsocket(0);
        sendTcpMessage(data, s, PSH | ACK, mqttData, 2);


        setTcpState(0, TCP_FIN_WAIT_1);
        setTcpState(1, MQTT_UNCONNECTED);
    }
}







void publishMqtt(char strTopic[], char strData[])
{
    if (getTcpState(0) == TCP_ESTABLISHED)
    {
        uint8_t mqttData[100];
        uint16_t mqttDataSize = 0;
        uint16_t topicLength = strlen(strTopic);
        uint16_t dataLength = strlen(strData);
        // MQTT publish packet
        // Variable headers
        mqttData[mqttDataSize++] = 0x30; // publish
        mqttData[mqttDataSize++] = 2 + topicLength + dataLength; // remaining length

        mqttData[mqttDataSize++] = 0x00; // topic length MSB
        mqttData[mqttDataSize++] = topicLength; // topic length LSB
        strcpy((char*)&mqttData[mqttDataSize], strTopic); // topic
        mqttDataSize += topicLength;
        strcpy((char*)&mqttData[mqttDataSize], strData); // data
        mqttDataSize += dataLength;

        // make ether
        uint8_t buffer[MAX_PACKET_SIZE];
        etherHeader *data = (etherHeader*) buffer;
        sendTcpMessage(data, getsocket(0), PSH | ACK, mqttData, mqttDataSize);
    }
}

void subscribeMqtt(char strTopic[])
{
    if (getTcpState(0) == TCP_ESTABLISHED)
    {
        uint8_t mqttData[100];
        uint16_t mqttDataSize = 0;
        uint16_t topicLength = strlen(strTopic);

        // MQTT subscribe packet
        mqttData[mqttDataSize++] = 0x82; // subscribe header
        mqttData[mqttDataSize++] = topicLength + 5; // remaining length = 2 (packet id) + 2 (topic length) + topicLength + 1 (QoS)
        mqttData[mqttDataSize++] = 0x00; // packet id MSB
        mqttData[mqttDataSize++] = 0x01; // packet id LSB
        mqttData[mqttDataSize++] = 0x00; // topic length MSB
        mqttData[mqttDataSize++] = topicLength; // topic length LSB

        strcpy((char*)&mqttData[mqttDataSize], strTopic); // topic
        mqttDataSize += topicLength;

        mqttData[mqttDataSize++] = 0x00; // QoS level (0)

        //Ethernet Packet
        uint8_t buffer[MAX_PACKET_SIZE];
        etherHeader *data = (etherHeader*) buffer;
        sendTcpMessage(data, getsocket(0), PSH | ACK, mqttData, mqttDataSize);
    }
}


void unsubscribeMqtt(char strTopic[])
{
    if (getTcpState(0) == TCP_ESTABLISHED)
    {
        uint8_t mqttData[100];
        uint16_t mqttDataSize = 0;
        uint16_t topicLength = strlen(strTopic);

        //Mqtt Disconnect pakcet
        mqttData[mqttDataSize++] = 0xA2; // unsubscribe
        mqttData[mqttDataSize++] = 2 + 2 + topicLength; // remaining length


        mqttData[mqttDataSize++] = 0x00; // packet id MSB
        mqttData[mqttDataSize++] = 0x01; // packet id LSB
        mqttData[mqttDataSize++] = 0x00; // topic length MSB
        mqttData[mqttDataSize++] = topicLength; // topic length LSB
        strcpy((char*)&mqttData[mqttDataSize], strTopic); // topic
        mqttDataSize += topicLength;

        // make ether
        uint8_t buffer[MAX_PACKET_SIZE];
        etherHeader *data = (etherHeader*) buffer;
        sendTcpMessage(data, getsocket(0), PSH | ACK, mqttData, mqttDataSize);
    }
}

void handleMqttPublish(char *topic, char *payload)
{
    if (strstr(topic, "irriStop") != NULL ||
        strstr(topic, "irriStart") != NULL ||
        strstr(topic, "irriPause") != NULL ||
        strstr(topic, "irriHazard") != NULL)
    {
        static char commandData[32]; // Safe buffer

        if (strstr(topic, "irriHazard") != NULL)
        {
            if (payload[0] == '1')
                strcpy(commandData, "on");
            else
                strcpy(commandData, "off");

            processCommand(topic, commandData);
        }
        else
        {
            // Pass the payload directly for zones/time, but double check size
            strncpy(commandData, payload, sizeof(commandData) - 1);
            commandData[sizeof(commandData) - 1] = '\0'; // Null terminate just in case

            processCommand(topic, commandData);
        }
    }
}


