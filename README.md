### ReplicatedLog app
[ReplicatedLog](src/ReplicatedLogApp/ReplicatedLog.cpp) app can be run in "master" and in "secondary" modes depending on command line parameters.

When run in master mode, synchronous replication to secondaries is performed.

When run in secondary mode, random delay is performed when receiving message. 

Master/Secondary communication is performed using HTTP requests.

### Usage example:

```sh
# Run "master" listening to 8000 port and register ports 8005 and 8006 for secondaries
./ReplicatedLog 8000 8005 8006 > master.txt &

# Run "secondary" listening to 8005 port
./ReplicatedLog 8005 > secondary1.txt &

# Run "secondary" listening to 8006 port
./ReplicatedLog 8005 > secondary2.txt &

curl localhost:8000 # -> []
curl localhost:8005 # -> []
curl localhost:8006 # -> []

# Post message to master
curl -d "message1" -X POST localhost:8000

# master.txt logs
| Post message: message1
| Executing curl cmd "curl -d "message1" -X POST "http://localhost:8005""...
| Executing curl cmd "curl -d "message1" -X POST "http://localhost:8006""...
| Reply: "Message "message1" accepted"
| Response from secondary (http://localhost:8006): Message "message1" accepted
| Reply: "Message "message1" accepted"
| Response from secondary (http://localhost:8005): Message "message1" accepted
| Timer Post request handling: 2688ms.

# secondary1.txt logs
| Post message: message1
| Delay for 2673ms
| Timer Post request handling: 2678ms.

# secondary2.txt logs
| Post message: message1
| Delay for 2370ms
| Timer Post request handling: 2375ms.

# GET requests
curl localhost:8000 # -> [message1]
curl localhost:8005 # -> [message1]
curl localhost:8006 # -> [message1]

```
