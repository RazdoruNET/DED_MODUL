#define ARRAY_SIZE(ar) sizeof(ar)/sizeof(ar[0])

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  if (type == WStype_TEXT && length > 0)
  {
    String dataStr = String((char*)payload);

    StaticJsonBuffer<5000> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(dataStr);

    if (json.success()) {

      int count = ARRAY_SIZE(angles);

      if (json["event"] != "ping")
      {
        cart         = (int) json["data"]["card"];
        rpm_new      = (int) json["data"]["rpm"]; 
        max_rpm      = (int) json["data"]["max_rpm"];
        max_cart_rpm = (int) json["data"]["angles"][count-1][0].as<int>();

        for (int i = 0; i < count; i++) 
        {         
            angles[i] = json["data"]["angles"][i][1].as<float>();
            rpms[i]   = json["data"]["angles"][i][0].as<float>();
        }
      }
    }

  }
}

void webSocketInit()
{
  webSocket.begin(ADDR, PORT, URL);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(500);
}
