var http = require('http');

http.get("http://www.gzjd.gov.cn/cgs/html/violation/visitor.html", function(res) {
  // console.log("Got response: " + res.body);

	// console.log('STATUS: ' + res.statusCode);
  console.log('HEADERS: ' + JSON.stringify(res.headers));
  res.setEncoding('utf8');
  res.on('data', function (chunk) {
    console.log('BODY: ' + chunk);
  });

}).on('error', function(e) {
  console.log("Got error: " + e.message);
});


