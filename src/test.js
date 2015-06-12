var http = require('http');
var iconv = require('iconv-lite');
var querystring = require('querystring');


var postData = querystring.stringify({
  'hpzl': '小型汽车',
  'hpzl': '粤A265B7',
  'fdjh': '7919',
  'clsbdh': '130190',
  'captcha': ''
});

var options = {
  hostname: 'www.gzjd.gov.cn',
  port: 80,
  path: '/cgs/vehiclelicense/checkVisitorVehicle.htm',
  method: 'POST',
  // charset: 'gb2312',
  headers: {
    'Content-Type': 'application/x-www-form-urlencoded',
    'Content-Length': postData.length
  }
};

var req = http.request(options, function(res) {
  // console.log('STATUS: ' + res.statusCode);
  if (res.statusCode === 200) {
    console.log('HEADERS: ' + JSON.stringify(res.headers));
    res.on('data', function (data) {
      console.log('BODY: ' + iconv.decode(data, 'gb2312'));
    });
    
  }
});

req.on('error', function(e) {
  console.log('problem with request: ' + e.message);
});

// write data to request body
req.write(postData);
req.end();
