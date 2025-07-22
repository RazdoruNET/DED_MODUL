void initRecoveryServer() 
{
  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request)
  {
    request->redirect("/update");
  });
  
  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404, "text/plain", "NOT FOUND ROUTE");
  });
  
  ElegantOTA.begin(&server);
  
  server.begin();

  Serial.println("HTTP server started");
}
