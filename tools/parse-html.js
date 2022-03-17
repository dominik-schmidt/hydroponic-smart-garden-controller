var fs = require('fs');

var filename = process.argv[2];
var site = process.argv[3];

fs.readFile(filename, 'utf8', function(err, data) {
  if (err) throw err;

  var parsed = data.replace(/"/g, "'");
  parsed = parsed.replace(/(<link .* href=')([^:]*)('.*>)/, `$1${site}/$2$3`);

  parsed = parsed.split('\n').map((line) => {
    if (line.match(/.*<a.*class='btn'.*<\/a>/)) {
      return line.replace(/\s*<a.*href='(.*)'.*>(.*)<\/a>/, '  htmlButton(client, "$1", "$2");\n');
    } else if (line.match(/.*<div.*id='light'.*<\/div>.*/)) {
      return '  htmlLightState(client);\n';
    } else if (line.match(/.*<div.*id='container'.*>.*/)) {
      return '  htmlPumpState(client);\n';
    }else {
      return line.replace(/\s*(.+)/, '  client->println("$1");\n');
    }
  }).join('');

  console.log(`void htmlDoc(WiFiClient* client) {\n${parsed}}`);
});
