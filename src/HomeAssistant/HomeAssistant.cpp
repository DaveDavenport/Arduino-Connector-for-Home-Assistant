#include "HomeAssistant.h"

#include "Arduino.h"
#include "ArduinoJson.h"
#include "HomeAssistantService.h"
#include <HTTPClient.h>

HomeAssistant::HomeAssistant() { }

/*
 * Sets the connection parameters for the HA API and WiFi connection
 * @param const char* ssid: Wifi SSID
 * @param const char* wifi_pass: Wifi Password
 * @param String api_base: Base url for the home assistant server
 * @param String api_pass: Password (if any) for the home assistant server
 */
void HomeAssistant::begin(String api_base, String api_pass) {
  if (api_base.endsWith("/")) {
    _API_BASE = api_base + "api/";
  } else {
    _API_BASE = api_base + "/api/";
  }
  if (api_pass != "") {
    _API_PASS = api_pass;
    _API_NO_PASS = false;
  }
}


/* Alias for the healthCheck method to determine if the API is connected */
bool HomeAssistant::connected()
{
  return healthCheck();
}

/*
 * Reads the state from a given entity
 * @param const String entity_id: Entity to read the state from
 * @return String: The current state of the entity
 */
String HomeAssistant::readState(const String entity_id)
{
  String response = readResponse(entity_id);
  return stateFromString(response);
}

/*
 * Reads the response payload from a given entity
 * @param const String entity_id: Entity to read the response from
 * @return JsonObject&: The state and attributes json from the entity
 */
String HomeAssistant::readResponse(const String entity_id)
{
  String url = _API_BASE + "states/" + entity_id;
  return get(url);
}

/*
 * Performs a POST request to change the state of an entity
 * @param String entity_id: Entity ID to change the state for
 * @param String payload: Custom payload to set the state and attributes
 * @return bool: True if successful
 */
bool HomeAssistant::setEntityStateWithPayload(const String entity_id, const String payload) {
  String url = _API_BASE + "states/" + entity_id;
  return post(url, payload);
}

/*
 * Performs a POST request to change the state of an entity
 * @param String state: State to change the entity to
 * @param String entity_id: Entity ID to change the state for
 * @return bool: True if successful
 */
bool HomeAssistant::setEntityState(const String state, const String entity_id) {
  String payload = stateJson(state);
  return setEntityStateWithPayload(entity_id, payload);
}

/*
 * Performs a POST request to a service with a payload and returns true if successful
 * @param uint8_t service: Endpoint to call in HA API
 * @param String payload: Custom payload to send to the post request
 * @return bool: True if successful
 */
bool HomeAssistant::callService(uint8_t service, String payload)
{
  HomeAssistantService ha_service(service);
  String url = _API_BASE + "services/" + ha_service.endpoint;
  return post(url, payload);
}

/*
 * Performs a POST request to a service and returns true if successful
 * @param uint8_t service: Endpoint to call in HA API
 * @param String entity_id: Entity to perform the action on
 * @return bool: True if successful
 */
bool HomeAssistant::callEntityService(uint8_t service, const String entity_id)
{
  String payload = serviceJson(entity_id);
  return callService(service, payload);
}

/* Checks to see if the API connection to HA is valid */
bool HomeAssistant::healthCheck()
{
  String jsonStr = get(_API_BASE);
  return jsonStr != "";
}

/* ******************************************************** */
/* **************** BEGIN JSON GENERATION ***************** */
/* ******************************************************** */

/*
 * Creates the JSON for a state request
 * @param String state: State to include in the payload
 * @return JsonObject&: Final state json payload
 */
String HomeAssistant::stateFromString(const String payload)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if(!root.success()) {
    return "FAILED TO PARSE RESPONSE";
  }
  const char* state = root["state"];
  return state;
}

/*
 * Creates the JSON for a state request
 * @param String state: State to include in the payload
 * @return JsonObject&: Final state json payload
 */
String HomeAssistant::stateJson(const String state)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["state"] = state;
  String payload;
  root.printTo(payload);
  return payload;
}

/*
 * Creates the JSON for a service request
 * @param String entity_id: Entity ID of the service to call
 * @return JsonObject&: Final service json payload
 */
String HomeAssistant::serviceJson(const String entity_id)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["entity_id"] = entity_id;
  String payload;
  root.printTo(payload);
  return payload;
}

/* ******************************************************** */
/* **************** BEGIN HTTP CLIENT ********************* */
/* ******************************************************** */

/*
 * Creates and returns a HTTPClient from a URL with authentication
 * @param HTTPClient* http: Pointer of http client to modify
 * @param String url: URL to call
 * @return HTTPClient
 */
void HomeAssistant::setupHttpClient(HTTPClient* http, const String url)
{
  http->begin(url);
  http->addHeader("Content-Type", "application/json");
  if (!_API_NO_PASS) { http->addHeader("x-ha-access", _API_PASS); }
}

/*
 * Performs a POST request for a given URL
 * @param String url: Url to call for the POST request
 * @param JsonObject& body: Body json to attach
 * @return JsonObject* : Json response of the GET request
 */
bool HomeAssistant::post(const String url, const String body) {
  HTTPClient http;
  setupHttpClient(&http, url);
  int httpCode = http.POST(body);
  return httpCode == HTTP_CODE_OK;
}

/*
 * Performs a GET request for a given URL
 * @param String url: Url to call for the GET request
 * @return String : Json response of the GET request
 */
String HomeAssistant::get(const String url) {
  HTTPClient http;
  setupHttpClient(&http, url);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    return http.getString();
  } else {
    return "";
  }
  http.end();
}
