

struct WifiParameters {
   uint32_t config_changed;
   char     *wifi_mode;
   char     *wifi_ssid;
   char     *wifi_key;

   WifiParameters () :
      config_changed (0),
      wifi_mode (NULL),
      wifi_ssid (NULL),
      wifi_key (NULL) {}

   ~WifiParameters ()
   {
      delete[] wifi_mode;
      delete[] wifi_ssid;
      delete[] wifi_key;
   }

   void set_wifi_mode (const char *mode)
   {
      delete[] wifi_mode;
      wifi_mode = NULL;
      if (mode) {
         wifi_mode = amstrdup (mode);
      }
   }

   void set_wifi_ssid (const char *ssid)
   {
      delete[] wifi_ssid;
      wifi_ssid = NULL;
      if (ssid) {
         wifi_ssid = amstrdup (ssid);
      }
   }

   void set_wifi_key (const char *key)
   {
      delete[] wifi_key;
      wifi_key = NULL;
      if (key) {
         wifi_key = amstrdup (key);
      }
   }
};