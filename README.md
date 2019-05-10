# Stream Login

A simple node server for remote forwarding of a customizable link.
The optional desktop frontend allows you to modify the remote link
at start up and shutdown.

## How it works

Either clone this repository or download its tarball.

In the `node` folder, copy the `configbase.json` to `config.json` and edit as
appropriate.  Please change the defaults to something suitable.  For example:

```
{
    "header": "First Line Portal of<br>Second Line Pty Ltd",
    "footer": "Terms of Use: Personal use only.",
    "welcome": "Welcome",
    "formUser": "username",
    "formPass": "password",
    "login": "LOGIN",
    "incorrect": "Incorrect login",
    "success": "Please click the folowing link",
    "urlAssignSecret": "KLJASD8ujkijhgnq4y5LKGDSFHJ"
}
```
Note that `urlAssignSecret` should be sufficently long, random, and plain
text.  It should be from the range [0-9,A-Z,a-z] and contain more than 16
letters.  Do not use the example secret above, make your own.  Mashing the
keyboard and removing non-alphanumeric characters should be fine.  You may
also consider visiting [random.org](https://www.random.org/strings/) and
joining a few strings together.

Likewise, copy `usersbase.json` to `users.json` and edit as appropriate.  For
example, the following will provide a simple stream link.

```
{
  "mystream": {
    "password": "mypassword",
    "link": "http://5.6.7.8:8000/stream",
    "text": "mp3 stream",
    "rel": "audio"
  }
}
```

The username and password probably do not need to be as secure, but keep
discretion, security and useablity in mind.  If you are using the desktop
client, consider making this an empty dictionary (`{ }`) instead.

Run the server on a node server in the cloud, or on a local box.

>cd node

>node server.js \[*myport* (optional)\]

Opening your browser and logging in with the correct username and password
will display the configured url.

The url can be changed live by making a POST request to
`http://myserver.somewhere/urlAssignSecret`, with the following (example) data:

```
{
  'action': 'set',
  'username': 'mystream',
  'password': 'mypassword',
  'tld': 'my_public_ip_address',
  'port': '8000',
  'path': 'path/to/my/stream.mp3',
  'text': 'link text',
  'rel': 'href'
}
```

When the password field is an empty string, the account is removed from the
server.

## Desktop client

The desktop client provides a means of setting some of the remote's live urls
to something meaningful when the system is turned on, and something else when
it is turned off.

### Winapi desktop client

Accounts to change are stored inside an `ini` subfolder, with one ini file
per account.  These ini-files are hand-crafted in a text editor.  An ini
file would look like this for the examples given above, if someone were
hosting a stream on their desktop.

```
[General]
remote=http://myremote.server/KLJASD8ujkijhgnq4y5LKGDSFHJ
username=mystream

[Active]
#link=http://my.remote.server.ip:8000/stream
port=8000
path=path/to/my/stream.mp3
text=mp3 stream

[Inactive]
#link=inactive.link.website
#port=80
#path=/
text=stream inactive
```

If `link` is not set, the desktop client creates a link using its own public
ip address (looked up with with `http://ifconfig.co/ip`) and the information
in the `port` and `path` fields, and passes it to the remote server.

If `link`, `port` and `path` are not set, the server will create a link to
itself.

This desktop client is only partially writen.

### Qt desktop client

Accounts are stored in an xml file in the program's executable directory.
These accounts may be edited from within the program.

Each account has two states, active and inactive.  Each state has a display
text field, url field, path-related fields, and checkboxes indicating which ip
address the path-related modes will use.

* type. The sort of information to be presented. (text, link, audio, video)
* text. The display text associated with the state.
* url.  The username's link (if used) will have this url
* path+port.  Same, but use a path+port on a specific ip (client or server)
* path.  Same, but use a path on a specific ip (client or server)
* ip.   The path(+path) should use the client or server address.

path+port and path options in conjuction with client ip are useful for
port-forwarding using your router.  path+port and path options with server ip
are useful if your server is also hosting the link.
