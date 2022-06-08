#include <Arduino.h>
#include <Network.hpp>
#include <WiFi.h>

// Set these to your desired credentials.
const char* Apssid = "PIA-NICOLAU-MARISOIU";
const char* Appassword = "123456789"; // (minimum length 8 required)

hsd::tcp_server_v4* server = nullptr;     

int button = 25;
int led = 12;
bool on = false;

template <typename T>
static inline std::pair<bool, long> manage_socket(T& socket);

void setup()
{
    // put your setup code here, to run once:

    pinMode(button, INPUT);
    pinMode(led, OUTPUT);

    Serial.begin(9600);
    Serial.println();
    Serial.println("Configuring access point...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(Apssid, Appassword);
    IPAddress myIP = WiFi.softAPIP();
    
    // Default IP is 192.168.4.1
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    Serial.println("Scan For Wifi in your Mobile or laptop, you will see this network");
}

void loop()
{
    if (server == nullptr && WiFi.softAPgetStationNum() != 0)
    {
        server = new hsd::tcp_server_v4("0.0.0.0:54000");
        Serial.println("Server started");
    }

    if (server != nullptr)
    {
        server->poll();

        for (auto socket = server->begin(); socket != server->end();)
        {
            on = (digitalRead(button) == HIGH) ? !on : on;
            auto [to_erase, state] = manage_socket(*socket);

            if (state < 0)
            {
                Serial.printf("Error: %s\n", server->error_message());
            }
            else if (to_erase)
            {
                socket = server->erase(socket);
            }
            else
            {
                ++socket;
            }
        }
    }
    else
    {
        on = (digitalRead(button) == HIGH) ? !on : on;
        digitalWrite(led, on);
    }

    delay(300);
}

template <typename T>
static inline std::pair<bool, long> send_html(const std::string_view& request, T& socket)
{
    using namespace std::string_literals;

    static constexpr const char* fmt = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n\r\n%s";

    static char stream[16384]{};
    auto size = snprintf(stream, 16384, fmt, request.size() + 1, request.data());

    auto send_state = socket.send(stream, size + 1);
    memset(stream, 0, sizeof(stream));
    
    if (send_state > 0)
    {
        return {true, send_state};
    }

    return {false, -1};
}

template <typename T>
static inline std::pair<bool, long> notify(T& socket)
{
    using namespace std::string_view_literals;

    static constexpr auto on_header = "HTTP/1.1 200 OK\r\n"
        "Content-Length: 7\r\n"
        "Content-Type: text\r\n"
        "\r\npornit"sv;

    static constexpr auto off_header = "HTTP/1.1 200 OK\r\n"
        "Content-Length: 6\r\n"
        "Content-Type: text\r\n"
        "\r\noprit"sv;

    digitalWrite(led, on);

    return {
        true, socket.send(
            on ? on_header.data() : off_header.data(), 
            on ? on_header.size() + 1 : off_header.size() + 1
        )
    };
}

template <typename T>
static inline std::pair<bool, long> manage_socket(T& socket)
{
    using namespace std::string_view_literals;
    static char recv_buffer[4096]{};

    memset(recv_buffer, 0, sizeof(recv_buffer));
    auto recv_state = socket.receive(recv_buffer, sizeof(recv_buffer));
            
    if (recv_state > 0)
    {     
        if (auto* req = strstr(recv_buffer, "GET"); req != nullptr)
        {
            if (req[4] == '/')
            {
                return send_html(
                    "<!DOCTYPE html>"
                    "<html>"
                    "   <head>"
                    "       <meta charset=\"UTF-8\">"
                    "           <title>Proiect Informatică Aplicată</title>"
                    "   </head>"
                    "   "
                    "   <script>"
                    "       var xhr = new XMLHttpRequest();"
                    "       var on = false;"
                    "       xhr.onreadystatechange = function () {"
                    "           if (xhr.readyState === 4) {"
                    "               document.getElementById('status').innerHTML = 'Status: ' + xhr.responseText;"
                    "               if (xhr.responseText.includes('pornit')) {"
                    "                   on = true;"
                    "               } else {"
                    "                   on = false;"
                    "               }"
                    "           }"
                    "       };"
                    "       "
                    "       function resolve() {"
                    "           return new Promise("
                    "               resolve => {"
                    "                   setTimeout(() => {"
                    "                       xhr.open('POST', 'http://192.168.4.1:54000/status');"
                    "                       xhr.setRequestHeader('Accept', 'text');"
                    "                       xhr.setRequestHeader('Content-Type', 'text');"
                    "                       xhr.send('status');"
                    "                       resolve();"
                    "                   }, 2000);"
                    "               }"
                    "           );"
                    "       }"
                    "       "
                    "       async function asyncCall() {"
                    "         	while (true) {"
                    "         		await resolve();"
                    "       	}"
                    "       }"
                    "       "
                    "       asyncCall();"
                    "       "
                    "       function toggle() {"
                    "           if (on) {"
                    "               xhr.open('POST', 'http://192.168.4.1:54000/status');"
                    "               xhr.setRequestHeader('Accept', 'text');"
                    "               xhr.setRequestHeader('Content-Type', 'text');"
                    "               xhr.send('off');"
                    "           } else {"
                    "               xhr.open('POST', 'http://192.168.4.1:54000/status');"
                    "               xhr.setRequestHeader('Accept', 'text');"
                    "               xhr.setRequestHeader('Content-Type', 'text');"
                    "               xhr.send('on');"
                    "           }"
                    "       }"
                    "   </script>"
                    "   <body style=\"background-color: rgb(52, 52, 52);\">"
                    "      <h1 style=\"color: white; text-align: center;\">Testează butonul</h1>"
                    "      "
                    "      <div style=\"text-align: center; height: 800px\">"
                    "          <button style=\"border-radius: 15px; width: fit-content;"
                    "          padding: 0 10px; color: white; margin-top: 10px;"
                    "          border-color: #9904f5; background-color: rgb(52, 52, 52);\""
                    "          id=\"status\" onclick=\"toggle(); return false;\">"
                    "              Status: oprit"
                    "          </button>"
                    "      </div>"
                    "   </body>"
                    "</html>"sv,
                    socket
                );
            }
        }
        else if (auto* req = strstr(recv_buffer, "POST"); req != nullptr)
        {
            auto* data = strstr(recv_buffer, "\r\n\r\n");

            if (data != nullptr)
            {
                data += 4;

                if (auto* req = strstr(data, "on"); req != nullptr)
                {
                    on = true;
                    return notify(socket);
                }
                else if (auto* req = strstr(data, "off"); req != nullptr)
                {
                    on = false;
                    return notify(socket);
                }
                else if(auto* req = strstr(data, "status"); req != nullptr)
                {
                    return notify(socket);
                }
            }
        }
    }
    
    return {false, recv_state};
}