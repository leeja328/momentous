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
      },
      {
        "type": "select",
        "messageKey": "HourWeight",
        "defaultValue": "2",
        "label": "Hour Weight",
        "options": [
          { "label": "Thin", "value": "0" },
          { "label": "Regular", "value": "1" },
          { "label": "Thick", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "MinuteWeight",
        "defaultValue": "0",
        "label": "Minute Weight",
        "options": [
          { "label": "Thin", "value": "0" },
          { "label": "Regular", "value": "1" },
          { "label": "Thick", "value": "2" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Info"
      },
      {
        "type": "select",
        "messageKey": "DateFormat",
        "defaultValue": "0",
        "label": "Date (Top)",
        "options": [
          { "label": "None", "value": "0" },
          { "label": "MM/DD", "value": "1" },
          { "label": "DD/MM", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BottomInfo",
        "defaultValue": "0",
        "label": "Bottom",
        "options": [
          { "label": "None", "value": "0" },
          { "label": "Temperature", "value": "1" },
          { "label": "Step Count", "value": "2" }
        ]
      },
      {
        "type": "radiogroup",
        "messageKey": "TempUnit",
        "defaultValue": "F",
        "label": "Temperature Unit",
        "options": [
          { "label": "Fahrenheit", "value": "F" },
          { "label": "Celsius", "value": "C" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
