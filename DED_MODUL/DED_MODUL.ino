#include "DED_CORE.h"

void setup()
{   
  initCore();
}

void loop()
{     
  loopCore();

  Serial.println("cart: " + String(cart));
  Serial.println("rpm: " + String(rpm));
  Serial.println("max_rpm: " + String(max_rpm));
  Serial.println("max_cart_rpm: " + String(max_cart_rpm));
}
