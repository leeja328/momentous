// Values match the COMPLICATION_* ids in momentous.c
function corner(messageKey, label) {
  return {
    "type": "select",
    "messageKey": messageKey,
    "defaultValue": "0",
    "label": label,
    "options": [
      { "label": "None", "value": "0" },
      { "label": "Weather", "value": "1" },
      { "label": "Battery", "value": "2" },
      { "label": "Day of Week", "value": "3" },
      { "label": "Day of Month", "value": "4" },
      { "label": "Steps", "value": "5" }
    ]
  };
}

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
      },
      {
        "type": "color",
        "messageKey": "ComplicationColor",
        "defaultValue": "0xAAAAAA",
        "label": "Complication Color"
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
        "defaultValue": "Complications"
      },
      corner("TopLeft", "Top Left"),
      corner("TopRight", "Top Right"),
      corner("BottomLeft", "Bottom Left"),
      corner("BottomRight", "Bottom Right"),
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
