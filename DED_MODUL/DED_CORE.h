#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <ElegantOTA.h>
#include "WebSocketsClient.h"

AsyncWebServer server(80);

#include "PROPS.h"

#include "WEB_SOCKET_CODE.h"
#include "WEB_SERVER.h"

Preferences preferences;

void initCore()
{

  // Инициируем серийный порт для отладки кода
  Serial.begin(115200);
  
  // Устанавливаем частоту процессора на 160 мигагерц
  setCpuFrequencyMhz(160);
  
  // Подгружаем настройки
  preferences.begin("ded_modul", false);

  // Подгружаем счетчик попыток запуска из настроек и плюсуем после чего кладем обратно
  int count_run = preferences.getInt("count_run",0) + 1;
  preferences.putInt("count_run", count_run);

  // Подгружаем счетчик успешных запусков
  int count_run_succes = preferences.getInt("count_run_succes",0);

  //Проверяем раздницу между неудачными попытками и удачными
  if (count_run - count_run_succes > 10)
  {
    // если разница больше > 10 переключаем режим модуля на FIX и сбрасываем настройки синхронизации и записываем в настройки статус режима fix
    preferences.putBool("fix_mode", true);
    preferences.putString("ssid", "");
    preferences.putString("password", "");
  }
  else
  {
    // если нет кладем в настройки диактив режима fix
    preferences.putBool("fix_mode", false);
  }

  // Проверяем активность режима fix
  if (preferences.getBool("fix_mode", false) == true)
  {
      //
      // Активирован режим fix 
      //

      Serial.println("FIX MODE ACTIVATE");

      // Сбрасываем счетчики запуска и записываем в настройки
      preferences.putInt("count_run",0);
      preferences.putInt("count_run_succes",0);
      
      // Запускаем вайфай точку доступа
      WiFi.mode(WIFI_AP);
      WiFi.softAP(fix_ssid, fix_password);

      // Инициируем веб сервер режима fix
      initRecoveryServer();
  } 
  else
  {
    //
    // Активирован обычный режим работы
    //

    // запрашиваем настройки подклбчения
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    // Переводим вайфай в режим модема
    WiFi.mode(WIFI_STA);

    // Если настройки пустые активируем подключение модуля к системе
    if (ssid == "")
    {
      // Сканируем WIFI
      int network_count = WiFi.scanNetworks();

      if (network_count == 0)
      {
        while (network_count == 0)
        {
          // ждем пока количество найденных точек будет больше 0
          network_count = WiFi.scanNetworks();
        }
      }
      
      // ищем точку доступа ded box в режиме восстановления

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

      // Подключаемся к найденной точке доступа      
      WiFi.begin(recovery_wifi_sid, recovery_wifi_password);
      
      delay(2000);

      Serial.println("CONNECT TO WIFI");

      // Получаем ip в локальной сети
      IPAddress localIP = WiFi.localIP();

      while (localIP.toString() == "0.0.0.0")
      {
        localIP = WiFi.localIP();
      }

      // Создаем Http клиента для запросов к ded box по сети
      HTTPClient http;

      // Делаем запрос к ded box что бы узнать название его точки доступа

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

      // Делаем запрос к ded box что бы узнать пароль его точки доступа

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

      // Делаем запрос к ded box что бы сказать ему перезагрузиться в обычный режим

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

    // Ожидаем успешное подключение и после активируем web socket
    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
      webSocketInit();
    }
  }

  // повышаем счетчик успешных запусков и записываем новые значения в настройки
  count_run_succes += 1;

  preferences.putInt("count_run",count_run_succes);
  preferences.putInt("count_run_succes",count_run_succes);
}

void loopCore()
{     
  // луп для ElegantOTA это обновления системы по воздуху
  ElegantOTA.loop();

  // Проверяем не активен ирежим fix
  if (preferences.getBool("fix_mode", false) == true)
  {
    // ЛУЧШЕ ОСТАВИТЬ ПУСТЫМ НО ЕСЛИ ОЧ ХОЧЕТСЯ ТО МОЖНО ВПИЗДЯЧИТЬ ЧТОНИБУДЬ
  }
  else
  {
    // Проверяем установлены ли настройки подключения
    if (ssid == "")
    {
       // ЛУЧШЕ ОСТАВИТЬ ПУСТЫМ НО ЕСЛИ ОЧ ХОЧЕТСЯ ТО МОЖНО ВПИЗДЯЧИТЬ ЧТОНИБУДЬ
    }
    else
    {      
      // луп для web socket
      webSocket.loop();
      
      // шлем пинг запрос на ded box что бы он нас не отрубил от web socet за бездействие
      if (micros()%1000 == 0)
      {
        webSocket.sendTXT("ping");
      }
    }
  }
}
