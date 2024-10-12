// TCP Server with WiFi connection communication via Socket
/*ESP-32 is the server here 
computer is the client here*/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_system.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"

#include "nvs_flash.h"
#include "ping/ping_sock.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "MFRC522.h"



#define PORT 3333
#define pass_size  5
#define name_size  8
#define LED_PIN_1 13
#define LED_PIN_2 2
static const char *TAG = "TCP_SOCKET";
static const char *TAG_RFID = "RFID";

//Used by RC522
uint8_t card_rx_buffer_pass[20];
uint8_t card_rx_buffer_name[20];
uint8_t card_rx_len = 20;
uint8_t req_buffer[16];
uint8_t req_len = 16;
//Used for the TCP recieving of name and password
char RCVD_NAME[8];
char RCVD_PASSWORD[5];
char Master_password[5] = {1,5,2,4,7}; // Assuming 5 characters + null terminator

//MiFare KEY
MIFARE_Key key = 
{
 .keyByte = {0xff,0xff,0xff,0xff,0xff,0xff}
};

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    char rx_buffer[128];
    char string_data[128];
    char data_to_send[128];
    int data_len, string_data_len;
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 5;
    int keepCount = 3;
    struct sockaddr_storage dest_addr;//defining struct to store the IPv4 and IPv6
    spi_device_handle_t spi = *(spi_device_handle_t*)pvParameters; // Get the spi handle from the passed parameter

    int counter = 0;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;// this casts the struct sockaddr_storage to sockaddr_in for IPv4 

    /*stores IP ADDRESS, INADDR_ANY means the server will accept connections on any IP address assigned to the device. htonl(host to long network) converts INADDR_ANY into BIG_Endian format
    AF_INET means that it is IPv4 socket
    htons(host to network short) comvert port number to big endian format */
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);

    // Open socket
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0); // 0, SOCK_STREAM for TCP Protocol
    int opt = 1;
    /*SOL_SOCKET refers to the level at which the option is applied, which is socket it self, 
    SO_REUSEADDR allows to reuse the same address and port cobination if the port was closed due to any reason
    &opt is the pointer the variable opt
    */
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //Print on screen that socket is created
    ESP_LOGI(TAG, "Socket created");
    /*Bind the socket to the typical IP address and port, This binds the socket (listen_sock) to the local IP address and port specified in the 
    dest_addr structure (configured earlier as INADDR_ANY for all IPs and the specified port, e.g., 3333).
    Takes input of Listen socket and cast the sockaddr_in to sockeraddr cause the bind() function accepts the input as sockaddr*/
    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    // Listen to the socket
    listen(listen_sock, 1); // Allows one device to connect

    while (1)
    {
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);//gives the size of structure of address of the clients

        // Accept socket
        /*It is the incoming connection from the client
        accept function accepts the generic stucture of socket information and address length
        */
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        /*TCP keep alive function helps to detect if the server is broken or in idle state
        1. Enable TCP keepalive mechanism on this socket (1 = enable)
        2. Set the idle time (in seconds) before the first keepalive probe is sent (5 seconds of inactivity)
        3. Set the interval time (in seconds) between successive keepalive probes (5 seconds between probes)
        4. Set the maximum number of keepalive probes to send before closing the connection if no response (3 probes) */
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)//PF_INET for IPv4 connection
        {   //converts the clients ip address from binary format into human readable format  and store it in addr_str
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }

        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        // Receive data from client
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }
        else if (len > 0)
        {
            rx_buffer[len] = 0; // Null-terminate the received data

            // Print the received message to the serial monitor
            printf("Received message: %s\n", rx_buffer);
            if (strncmp(rx_buffer, "RFID_ID", 7)== 0)
            {
                ESP_LOGI(TAG_RFID, "Extracting Name and PAASWORDS.............");
                memset(RCVD_NAME, 0, sizeof(RCVD_NAME));
                memset(RCVD_PASSWORD, 0, sizeof(RCVD_PASSWORD));
                // Copy NAME (8 characters after "RFID_Enroll:")
                strncpy(RCVD_NAME, rx_buffer + 8, name_size); // 7 +1 cause of colon
                strncpy(RCVD_PASSWORD, rx_buffer + 8 + name_size + 1, pass_size); // +1 to skip the colon (:)
                RCVD_NAME[name_size] = '\0'; // Null-terminate the name
                RCVD_PASSWORD[pass_size] = '\0'; // Null-terminate the password
                ESP_LOGI(TAG_RFID, "The name and password exteacted are %s and %s", RCVD_NAME, RCVD_PASSWORD);
                 PCD_Init(spi);
                    while (1) {
                        printf("TCP RFID");
                            if (PICC_IsNewCardPresent(spi)) {
                            GetStatusCodeName(PICC_Select(spi, &uid, 0));
                            ESP_LOGI(TAG_RFID,"Card selected");
                            // Getting authentication
                            GetStatusCodeName(PCD_Authenticate(spi, PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(uid)));
                            ESP_LOGI(TAG_RFID,"Card Authenticated");
                            //Writing name
                            GetStatusCodeName(MIFARE_Write(spi, 4, (uint8_t *)RCVD_NAME, 16));
                            ESP_LOGI(TAG_RFID,"Username written");
                            //Writing passcode
                            GetStatusCodeName(MIFARE_Write(spi, 5, (uint8_t *)RCVD_PASSWORD, 16));
                            ESP_LOGI(TAG_RFID, "Password written");
                            PCD_StopCrypto1(spi);
                            break;
                            }
                            vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100 ms
                      }




            }
            
            // Copy the response message into the string_data variable
            strcpy(string_data, "Response from ESP32 Server via Socket connection");
            // Get the length of the response message
            string_data_len = strlen(string_data);
            // Format the HTTP response header with a status line and content length
            // "HTTP/1.1 200 OK" indicates a successful request
            // Content-Length indicates the length of the response body
            sprintf(data_to_send, " HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", string_data_len);//this formats a string and store it in another string
            // Concatenate the response message to the HTTP response header
            strcat(data_to_send, string_data);
            printf("Concatination result : %s \n", data_to_send);
            // Get the total length of the complete response (header + body)
            data_len = strlen(data_to_send);
            // Send the complete HTTP response back to the client through the socket
            send(sock, data_to_send, data_len, 0);
        }

        counter++;
        printf("send_reply function number %d was activated\n", counter);
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        shutdown(sock, 0);
        close(sock);
    }
    close(listen_sock);
    vTaskDelete(NULL);
}

void rfidTask(void *pvPara) {
    spi_device_handle_t spi = *(spi_device_handle_t*)pvPara; // Get the spi handle from the passed parameter
    while (1) {
        //printf("while rfid");
        if (PICC_IsNewCardPresent(spi)) { // Checking for new card
            GetStatusCodeName(PICC_Select(spi, &uid, 0));
            memset(card_rx_buffer_name, 0, card_rx_len);
            memset(card_rx_buffer_pass, 0, card_rx_len);
            // Getting authentication
            GetStatusCodeName(PCD_Authenticate(spi, PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(uid)));
            // Read back username
            MIFARE_Read(spi, 4, card_rx_buffer_name, &card_rx_len);
            ESP_LOGI(TAG_RFID, "Name stored in MIFARE block %d : %s", 4, (char*)card_rx_buffer_name);
            // Read back password
            MIFARE_Read(spi, 5, card_rx_buffer_pass, &card_rx_len);
            ESP_LOGI(TAG_RFID, "Password stored in MIFARE block %d : %s", 5, (char*)card_rx_buffer_pass);
            // Stop communication to start reauthentication next time
            PCD_StopCrypto1(spi);
            if(strcmp("15247", (char*)card_rx_buffer_pass)== 0){
                ESP_LOGI(TAG_RFID,"Password Verified");
                esp_rom_gpio_pad_select_gpio(LED_PIN_1);
                gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);
                gpio_set_level(LED_PIN_1, 1); // Turn on the LED
                vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for 1 second
                gpio_set_level(LED_PIN_1, 0); // Turn off the LED
            }
            else if(strcmp("15247", (char*)card_rx_buffer_pass) != 0){
                ESP_LOGI(TAG_RFID,"Password NOT Verified");
                esp_rom_gpio_pad_select_gpio(LED_PIN_2);
                gpio_set_direction(LED_PIN_2, GPIO_MODE_OUTPUT);
                gpio_set_level(LED_PIN_2, 1); // Turn on the LED
                vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for 1 second
                gpio_set_level(LED_PIN_2, 0); // Turn off the LED

            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG_RFID,"Password written is RFID: %s ",(char*)card_rx_buffer_pass );
        

    }
}



static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting WIFI_EVENT_STA_START ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected WIFI_EVENT_STA_CONNECTED ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection WIFI_EVENT_STA_DISCONNECTED ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}



void wifi_connection()
{   //Initializing the NVS to store wifi credentials
    nvs_flash_init();
    //Initializing the network interface necessary before any kind of networking with esp-32
    esp_netif_init();
    //Creating a default loop to create events for handling wifi or other networking tasks
    esp_event_loop_create_default();
    // Default Wi-Fi station (STA) interface. It prepares the ESP32 to connect to a Wi-Fi network as a client (not an access point).
    esp_netif_create_default_wifi_sta();
    //Initizlizing the wifi with basic credintials needed for wifi connection
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_initiation);// Initialize the Wi-Fi driver with the specified configuration.
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);// Register the event handler to handle any Wi-Fi events.
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);// Register the event handler for the IP event when the station (STA) gets an IP address.
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "Work Station",
            .password = "Zedian123"}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration); // Apply the Wi-Fi configuration to the station interface.
    esp_wifi_set_mode(WIFI_MODE_STA); // Set the Wi-Fi mode to station, enabling the ESP32 to connect to a network.
    esp_wifi_start();// Start the Wi-Fi driver, making it ready to connect.
    esp_wifi_connect();// Initiate the connection process to the specified Wi-Fi network.
}

void app_main(void)
{
    wifi_connection();
    /*********RFID INITIALIZATION */
    // Initialize the SPI communication for RC522
    esp_err_t spi_ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = 19,
        .mosi_io_num = 23,
        .sclk_io_num = 18,
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5000000, // Clock out at 5 MHz
        .mode = 0,                 // SPI mode 0
        .spics_io_num = 5,         // CS pin
        .queue_size = 7            // We want to be able to queue 7 transactions at a time
    };
    // Initialize the SPI bus
    spi_ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(spi_ret == ESP_OK);
    // Attach the RFID to the SPI bus
    spi_ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    assert(spi_ret == ESP_OK);

    // Initialize the MFRC522
    PCD_Init(spi);
    /****************************** INITIALIZATION DONE */
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // Start RFID task and pass the spi handle as parameter
    xTaskCreate(rfidTask, "RFID Task", 4096, &spi, 5, NULL);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, &spi, 5, NULL);
}