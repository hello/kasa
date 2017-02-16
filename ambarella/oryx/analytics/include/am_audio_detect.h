
struct AudioDetectParameters {
   uint32_t config_changed;
   uint32_t audio_channel_number;
   uint32_t audio_sample_rate;
   uint32_t audio_chunk_bytes;
   uint32_t enable_alert_detect;
   uint32_t audio_alert_sensitivity;
   uint32_t audio_alert_direction;
   uint32_t enable_analysis_detect;
   uint32_t audio_analysis_direction;
   uint32_t audio_analysis_mod_num;
   audio_analy_param aa_param[MAX_AUDIO_ANALY_MOD_NUM];

};
