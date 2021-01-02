# NWFileTransfer
A Socket based file transfer assignment. 

## Requirements: 
1. Send Multiple file concurrently from client to Server 
2. Check Integrity by calculate checksum

## Task List
- [x] Implement NWClient and NWServer and create socket connection.
- [x] Exchange Sample message between client and server.
- [x] Send single text file from client.
- [x] Send multiple text files from client.
- [x] Read all files from client directory /res and send to server
- [x] Limit the receiving file to 10 at server for debug purpose. This will be change later.
- [ ] Get Clarfication from Dr. Arslan about TCP checkSum requirements.
- [ ] Develop utility functions for calculate checkSum at Client and Server program.
- [ ] Write Sample program about pThread to test concurrency.
- [ ] Add concurrency using pThread into client and server program.
- [ ] Use pack tracer to check concurrency at client and server interface.
- [ ] Rewrite the logic of sending/receiving file and implement logic for sending generic file not just text file.
- [ ] Test the prgramm by sending image or other type of files.
- [ ] Create a dataset with 100 files each 10MB size and transfer with concurrency 1, 2,4 and 8 and measure throughput. 
- [ ] Once the test is done, draw a figure for concurrency value vs throughput.




