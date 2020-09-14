#include "mqtt_client.h"
#include "lwip/pbuf.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/netif.h"
#include "string.h"
#include "device.h"
#include "my_config.h"
#include "lwip/altcp_tls.h"

extern uint8_t *macaddr;
extern dev_ctrl device;

static mqtt_client_t mqtt_client;
static uint8_t mqtt_recv_buffer[64];
//"/resp/c028e3525d2e"
static char resp_topic[20];

#if LWIP_DNS
extern ip_addr_t mqtt_server_ip;
static void do_connect(mqtt_client_t *client, ip_addr_t *serverip);
#else
static void do_connect(mqtt_client_t *client);
#endif

/* Called when publish is complete either with success or failure */
//static void mqtt_pub_request_cb(void *arg, err_t result)
//{
//  if(result != ERR_OK) {
//    printf("Publish result: %d\r\n", result);
//  }
//}

//static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
//{
//  printf("Incoming publish at topic %s with total length %u\r\n", topic, (unsigned int)tot_len);
//  
//  /* Decode topic string into a user defined reference */
//
//}

static void do_analysis_data(char *data) 
{
  __IO uint8_t point = 0;
  uint8_t cmd = 0;
  
  if (strncmp(data, "led", 3) == 0) {
    cmd = MQTT_CMD_TYPE_LED;
    point = 3;
  } 
  else if (strncmp(data, "temp", 4) == 0) {
    cmd = MQTT_CMD_TYPE_TEMP;
    point = 4;
  } 
  else if (strncmp(data, "window", 6) == 0) {
    cmd = MQTT_CMD_TYPE_WINDOW;
    point = 6;
  } else {
  
  }

  if (cmd) {
    if (strncmp((char *)(data + point), ":on", 3) == 0) {
      if (strlen((char *)(data + point)) == 3) {
        device.device_ctrl = DEVICE_OPEN;
        device.mqtt_cmd = cmd;
      }
    } else if (strncmp((char *)(data + point), ":off", 4) == 0) {
      if (strlen((char *)(data + point)) == 4) {
        device.device_ctrl = DEVICE_CLOSE;
        device.mqtt_cmd = cmd;
      }
    }
  }
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  //  printf("Incoming publish payload with length %d, flags %u\r\n", len, (unsigned int)flags);
  if(flags & MQTT_DATA_FLAG_LAST) {
    /* Last fragment of payload received (or whole part if payload fits receive buffer
    See MQTT_VAR_HEADER_BUFFER_LEN)  */  
    /* Don't trust the publisher, check zero termination */
//    if(data[len] == 0) {
    memset(mqtt_recv_buffer, 0, sizeof(mqtt_recv_buffer));
    memcpy(mqtt_recv_buffer, data, len);
    printf("mqtt_incoming_data_cb:[%s]\r\n", (const char *)mqtt_recv_buffer);
    if (strncmp((char *)mqtt_recv_buffer, "ctrl", 4) == 0) {
      do_analysis_data((char *)(mqtt_recv_buffer + 4 + 1));
    }
  } else {
    /* Handle fragmented payload, store in buffer, write to file or whatever */
  }
}

//static void mqtt_sub_request_cb(void *arg, err_t result)
//{
//  /* Just print the result code here for simplicity,
//  normal behaviour would be to take some action if subscribe fails like
//  notifying user, retry subscribe or disconnect from server */
//  printf("Subscribe result: %d\r\n", result);
//}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  err_t err;
  if(status == MQTT_CONNECT_ACCEPTED) {
    printf("mqtt_connection_cb: Successfully connected\r\n");
    /* Setup callback for incoming publish requests */
//    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);
    mqtt_set_inpub_callback(client, NULL, mqtt_incoming_data_cb, arg);
    
    /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */
    char ctrltopic[20];
    memset(ctrltopic, 0, sizeof(ctrltopic));
    snprintf(ctrltopic, 20, "/ctrl/%02x%02x%02x%02x%02x%02x",
             macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    err = mqtt_subscribe(client, ctrltopic, 0, NULL, arg);
    if(err != ERR_OK) {
      printf("mqtt_subscribe return: %d\r\n", err);
    }
  } else {
    /* Its more nice to be connected, so try to reconnect */
#if LWIP_DNS
    printf("mqtt_connection_cb: Disconnected, reason: %d ip:%s\r\n", status, ipaddr_ntoa((ip_addr_t *)arg));
    do_connect(client, (ip_addr_t *)arg);
#else
     printf("mqtt_connection_cb: Disconnected, reason: %d\r\n", status);
    do_connect(client);
#endif
  }
}

#if LWIP_DNS
static void do_connect(mqtt_client_t *client, ip_addr_t *serverip)
#else
static void do_connect(mqtt_client_t *client)
#endif
{
  struct mqtt_connect_client_info_t ci; 
  err_t err;
  //2*6 + 1
  char client_id[13];
  //2*6 + 1 + 6 + 1 + 1
  char lastwill_topic[21];
  
  /* Setup an empty client info structure */
  memset(&ci, 0, sizeof(ci));
  
  memset(client_id, 0, sizeof(client_id));
  memset(lastwill_topic, 0, sizeof(lastwill_topic));
  memset(resp_topic, 0, sizeof(resp_topic));
  
  snprintf(client_id, 13, "%02x%02x%02x%02x%02x%02x", 
           macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
  snprintf(lastwill_topic, 21, "/status/%02x%02x%02x%02x%02x%02x",
           macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
  snprintf(resp_topic, 19, "/resp/%02x%02x%02x%02x%02x%02x",
           macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
  
  printf("will topic:%s\r\n", lastwill_topic);
  ci.client_id = client_id;
  ci.will_topic = lastwill_topic;
  
  /* Minimal amount of information required is client identifier, so set it here */
  ci.will_qos = 0;
  ci.will_retain = 1;
  ci.will_msg = MQTT_MSG_OFFLINE;
  ci.keep_alive = MQTT_KEEPALIVE;
  ci.client_user = MQTT_USER;
  ci.client_pass = MQTT_PASSWORD;
#if LWIP_ALTCP
  ci.tls_config = altcp_tls_create_config_client(certfile, sizeof(certfile));
#endif
  /* Initiate client and connect to server, if this fails immediately an error code is returned
  otherwise mqtt_connection_cb will be called with connection result after attempting
  to establish a connection with the server.
  For now MQTT version 3.1.1 is always used */
#if LWIP_DNS
  err = mqtt_client_connect(client, serverip, MQTT_SERVER_PORT, mqtt_connection_cb, (void *)serverip, &ci);
#else
  const ip_addr_t mqtt_server_ip = IPADDR4_INIT_BYTES(MQTT_SERVER_IP_0, MQTT_SERVER_IP_1, MQTT_SERVER_IP_2, MQTT_SERVER_IP_3);
  err = mqtt_client_connect(client, &mqtt_server_ip, MQTT_SERVER_PORT, mqtt_connection_cb, 0, &ci);
#endif
    
  /* For now just print the result code if something goes wrong */
  if(err != ERR_OK) {
    printf("mqtt_connect return %d\r\n", err);
  } else {
    mqtt_publish(client, ci.will_topic, MQTT_MSG_ONLINE, strlen(MQTT_MSG_ONLINE), 0, 1, NULL, NULL);
  }
}

#if LWIP_DNS
void mqtt_init(ip_addr_t *serverip) {
  do_connect(&mqtt_client, serverip);
}
#else
void mqtt_init(void) {
  do_connect(&mqtt_client);
}
#endif

void do_mqtt_publish(char *pub_payload) {
  err_t err;
  if (mqtt_client_is_connected(&mqtt_client)) {
    err = mqtt_publish(&mqtt_client, resp_topic, pub_payload, strlen(pub_payload), 0, 0, NULL, NULL);
    if(err != ERR_OK) {
      printf("Publish err: %d\r\n", err);
    }
  } else {
    printf("mqtt not connected!\r\n");
  }
}