const http = require('http');
const url = require('url');
const fs = require('fs');
const path = require('path');
const port = process.argv[2] || 9000;
const config = require('./config.json')
var users = require('./users.json')
const ctTextPlain = "text/plain;charset=UTF-8";
console.log(config);

//var streamUrl = config.urlDefault;
function nothing(data) { return data; }

function filter(data) {
  var r = data.toString();
  for (var c in config) {
    r = r.replace('$'+c, config[c]);
  }
  return r;
}

function doPost(request, response, callback) {
    var queryData = "";
    if(typeof callback !== 'function') return null;

    if(request.method == 'POST') {
        request.on('data', function(data) {
            queryData += data;
            if(queryData.length > 1e6) {
                queryData = "";
                response.writeHead(413, {'Content-Type': ctTextPlain}).end();
                request.connection.destroy();
            }
        });
        request.on('end', function() {
            callback(request, response, queryData);
        });
    } else {
        response.writeHead(400, {'Content-Type': ctTextPlain});
        response.end('400 Bad Request');
    }
}

function doLogin(req, res) {
  doPost(req, res, function(req, res, data) {
    fields = JSON.parse(data);
    v = {};
    var ok = fields.username && fields.password &&
             (fields.username in users) &&
             users[fields.username].password === fields.password;
    var linkOk = ok && users[fields.username].link !== ''
                    && users[fields.username].rel === 'href';
    v.status = ok ? 'ok' : 'invalid';
    v.notice = !ok ? config.incorrect : '';
    v.message = ok && linkOk ? config.success : '';
    v.link = ok ? users[fields.username].link : '';
    if (ok && ("text" in users[fields.username])) {
      v.text = users[fields.username].text;
    }
    v.rel = "href";
    if (ok && ("rel" in users[fields.username])) {
      v.rel = users[fields.username].rel;
    }
    res.setHeader('Content-Type','application/json');
    res.end(JSON.stringify(v));
  });
}

function doAssign(req, res) {
  doPost(req, res, function(req, res, data) {
    fields = JSON.parse(data);
    var linkUrl;
    var linkText;
    console.log(fields);
    if (fields.action === 'set'
      && typeof fields.username === 'string'
      && typeof fields.password === 'string') {
      if (fields.password === '') {
        // user is removed if no password is set!        
        if (fields.username in users)
           delete users[fields.username]
      } else {
        obj = {}
        obj.password = fields.password;
        obj.text = (typeof fields.text === 'undefined') ? config.defaultText : fields.text;
        if (typeof fields.url !== 'undefined') {
          // link provided, so only look at that
          obj.link = fields.url;
        } else {
          // no link provided, construct one out of the other fields
          if (typeof fields.path === 'undefined')
            fields.path = '';
          if (typeof fields.port === 'undefined')
            fields.port = '80';
          if (typeof fields.tld === 'undefined')
            fields.tld = req.connection.remoteAddress;
          obj.link = `http://${fields.tld}:${fields.port}/${fields.path.replace(/^\/+/g, '')}`;
        }
        obj.rel = 'href';
        if (typeof fields.rel !== 'undefined') {
          obj.rel = fields.rel;
          if (obj.rel === 'link')
            obj.rel = 'href';
          if (obj.rel === 'text')
            obj.link = '#';
        }
        console.log(obj);
        users[fields.username] = obj;
      }
    }
    console.log(users);
    res.writeHead(200, {'content-type': 'text/plain'});
    res.end();
  });
}

function doFile(pathname, res, filter) {
  // based on the URL path, extract the file extention. e.g. .js, .doc, ...
  const ext = path.parse(pathname).ext;
  // maps file extention to MIME typere
  const map = {
    '.ico': 'image/x-icon',
    '.html': 'text/html;charset=utf-8',
    '.js': 'text/javascript;charset=utf-8',
    '.json': 'application/json',
    '.css': 'text/css;charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.wav': 'audio/wav',
    '.mp3': 'audio/mpeg',
    '.svg': 'image/svg+xml;charset=utf-8',
    '.pdf': 'application/pdf',
    '.doc': 'application/msword',
    '.txt': ctTextPlain
  };

  fs.exists(pathname, function (exist) {
    if(!exist) {
      // if the file is not found, return 404
      res.statusCode = 404;
      res.end(`File ${pathname} not found!`);
      return;
    }

    // if is a directory search for index file matching the extention
    if (fs.statSync(pathname).isDirectory()) pathname += '/index' + ext;

    // read file from file system
    fs.readFile(pathname, function(err, data){
      if(err){
        res.statusCode = 500;
        res.end(`Error getting the file: ${err}.`);
      } else {
        // if the file is found, set Content-type and send data
        res.setHeader('Content-type', map[ext] || 'text/plain' );
        res.end(filter(data));
      }
    });
  });  
}

http.createServer(function (req, res) {
  // parse URL
  const parsedUrl = url.parse(req.url);
  const urlParts = parsedUrl.pathname.split('/');
  // extract URL path
  if (urlParts[1] === "files")
    doFile(`files/${parsedUrl.pathname}`, res, nothing);
  else if (urlParts[1] === "")
    doFile('templates/index.html', res, filter);
  else if (urlParts[1] === "login")
    doLogin(req, res);
  else if (urlParts[1] === config.urlAssignSecret)
    doAssign(req, res);
  else
    doFile('', res, nothing);
}).listen(parseInt(port));

console.log(`Server listening on port ${port}`);
