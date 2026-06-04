function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Sheet1");
  var data = JSON.parse(e.postData.contents);

  sheet.appendRow([
    data.timestamp,
    data.event_type,
    data.fullness,
    data.before,
    data.after
  ]);

  return ContentService.createTextOutput("Success");
}