
enum MotionDetectParam {
  MD_ROI_LEFT = 0,
  MD_ROI_RIGHT,
  MD_ROI_TOP,
  MD_ROI_BOTTOM,
  MD_ROI_THRESHOLD,
  MD_ROI_SENSITIVITY,
  MD_ROI_VALID,
  MD_ROI_OUTPUT_MOTION,
  MD_ROI_PARAM_MAX_NUM,
};

struct MotionDetectParameters {
  uint32_t config_changed;
  char algorithm[8];
  uint32_t value[MAX_MOTION_DETECT_ROI_NUM][MD_ROI_PARAM_MAX_NUM];
};
