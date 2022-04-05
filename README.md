# csce-311-project-3-kyliegore

My current set of files are slated to do the following.

First, the server starts and creates a named semaphore. The server then blocks, and the client starts which the client therefore creates the shared memory. The client then copies the filename to shared memory. Then the server unblocks. The client then blocks and then the server uses the filepath of open the file. Then the lines are read into shared memory. The client unblocks and the server blocks. The client then copies lines straight from the shared memory and then loops through the file lines in order to find the desired string.
