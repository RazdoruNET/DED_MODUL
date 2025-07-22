#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>

Preferences preferences;

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

#include <ElegantOTA.h>

#include "WebSocketsClient.h"

#include "PROPS.h"

#include "WEB_SOCKET_CODE.h"
#include "TASK_CODE.h"

AsyncWebServer server(80);

#include "WEB_SERVER.h"

void initCore()
{
  Serial.begin(115200);
  
  setCpuFrequencyMhz(160);
  
  preferences.begin("ded_modul", false);

  int count_run = preferences.getInt("count_run",0) + 1;

  preferences.putInt("count_run", count_run);

  int count_run_succes = preferences.getInt("count_run_succes",0);

  if (count_run - count_run_succes > 10)
  {
    preferences.putBool("fix_mode", true);
    preferences.putString("ssid", "");
    preferences.putString("password", "");
  }
  else
  {
    preferences.putBool("fix_mode", false);
  }

  if (preferences.getBool("fix_mode", false) == true)
  {
      Serial.println("FIX MODE ACTIVATE");

      preferences.putInt("count_run",0);
      preferences.putInt("count_run_succes",0);

      WiFi.mode(WIFI_AP);
      WiFi.softAP(fix_ssid, fix_password);

      initRecoveryServer();
  } 
  else
  {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
      
    WiFi.mode(WIFI_STA);

    if (ssid == "")
    {
      int network_count = WiFi.scanNetworks();

      if (network_count == 0)
      {
        while (network_count == 0)
        {
          network_count = WiFi.scanNetworks();
        }
      }
      
      int status_rec = 0;

      String recovery_wifi_sid = "DED_RECOVERY";
      String recovery_wifi_password = "1234567890";

      while (status_rec == 0)
      {
        for (int i = 0; i < network_count; ++i) 
        {
          if (recovery_wifi_sid == WiFi.SSID(i))
          {
            status_rec = 1;
            i = network_count;
          }
        }

        network_count = WiFi.scanNetworks();
      }
      
      WiFi.begin(recovery_wifi_sid, recovery_wifi_password);
      
      delay(2000);

      Serial.println("CONNECT TO WIFI");

      IPAddress localIP = WiFi.localIP();

      while (localIP.toString() == "0.0.0.0")
      {
        localIP = WiFi.localIP();
      }

      HTTPClient http;

      String url_ssid = "http://192.168.4.1/wifi/ssid";
      
      http.begin(url_ssid.c_str());
        
      int http_ssid_ResponseCode = http.GET();
        
      if (http_ssid_ResponseCode>0) 
      {
        preferences.putString("ssid", http.getString());
      }
      else 
      {
        Serial.print("Error code: ");
        Serial.println(http_ssid_ResponseCode);
      }
    
      http.end();

      String url_pass = "http://192.168.4.1/wifi/pass";
      
      http.begin(url_pass.c_str());
        
      int http_pass_ResponseCode = http.GET();
        
      if (http_pass_ResponseCode>0) 
      {
        preferences.putString("password", http.getString());
      }
      else 
      {
        Serial.print("Error code: ");
        Serial.println(http_pass_ResponseCode);
      }
    
      http.end();

      String url_reboot = "http://192.168.4.1/core/reboot";
      
      http.begin(url_reboot.c_str());
        
      int http_reboot_ResponseCode = http.GET();
        
      if (http_reboot_ResponseCode>0) 
      {
        Serial.print("HTTP Response code: ");
        Serial.println(http_reboot_ResponseCode);
      }
      else 
      {
        Serial.print("Error code: ");
        Serial.println(http_reboot_ResponseCode);
      }
    
      http.end();

      ESP.restart();
    }
    else
    {
      WiFi.begin(ssid, password);
      while (WiFi.localIP().toString() == "0.0.0.0");
    }

    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
      webSocketInit();
    }
  }

  count_run_succes += 1;

  preferences.putInt("count_run",count_run_succes);
  preferences.putInt("count_run_succes",count_run_succes);
}


void loopCore()
{     
  ElegantOTA.loop();

  if (preferences.getBool("fix_mode", false) == true)
  {
    // ЛУЧШЕ ОСТАВИТЬ ПУСТЫМ НО ЕСЛИ ОЧ ХОЧЕТСЯ ТО МОЖНО ВПИЗДЯЧИТЬ ЧТОНИБУДЬ
  }
  else
  {
    if (ssid == "")
    {
       // ЛУЧШЕ ОСТАВИТЬ ПУСТЫМ НО ЕСЛИ ОЧ ХОЧЕТСЯ ТО МОЖНО ВПИЗДЯЧИТЬ ЧТОНИБУДЬ
    }
    else
    {      
      webSocket.loop();
      
      if (micros()%1000 == 0)
      {
        webSocket.sendTXT("ping");
      }
    }
  }
}
