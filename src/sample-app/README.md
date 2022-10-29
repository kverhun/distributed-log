Sample app implements web-server (using Mongoose).

Prints receive HTTP request body.

Intended usage for future log app:

`curl -d "message1" -X POST "http://localhost:8000"`
should act as a `POST` command for master.

`GET` command is a default HTTP GET request to a server:

`curl "http://localhost:8000"`
will return list of messages.
