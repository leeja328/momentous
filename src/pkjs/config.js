module.exports = [
  {
    "type": "heading",
    "defaultValue": "Momentous"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "HourColor",
        "defaultValue": "0xFFFFFF",
        "label": "Hour Color"
      },
      {
        "type": "color",
        "messageKey": "MinuteColor",
        "defaultValue": "0xAAAAAA",
        "label": "Minute Color"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Layout"
      },
      {
        "type": "toggle",
        "messageKey": "LargeFont",
        "defaultValue": false,
        "label": "Large Font",
        "description": "Hour fills the top half of the screen, minutes fill the bottom half."
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
