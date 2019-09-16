#Multi-Threaded Web Server

##I. How to compile and run
	1. Program set up:
		a. Open Terminal, input the <path> to the Multi-Threaded-Web-Server directory.
			%cd <path>
		b. Generate the web_server
			%make
	2. Set up the web_server
		a. Input  % ​./web_server <port> <path> <num_dispatch> <num_workers> 0 <qlen> <cache_entries>
		 For example, %./web_server 9000 /home/Project_3/testing 10 10 0 10 10
		b. The server will be configurable in the following ways:
				● <port​ number> use ports 1025-65535 by default
				● <path>​ the path to your web root location from where all the files will be served
				● <num_dispatcher> ​the number of dispatcher threads to start up
				● <num_workers​> the number of worker threads to start up
				● <qlen>​ the fixed, bounded length of the request queue
				● <cache_entries> the number of entries available in the cache
	3. Connect client with server
				a. After you run the server, open a new terminal to connect to the server.
				b. You can try to download a file using this command:
					%wget http://<local host>:<port number> <path to the file you want to download>
				For example,
				 	%​wget http://127.0.0.1:9000/image/jpg/29.jpg
				c. Alternatively, you can open your web browser (FireFox is recommended, while Chrome is less stable), input the following command in your address line:
					%http://<local host>:<port number> <path to the file you want to download>
				Then the file you are requested will download automatically.

##II. How this program works
		This multi-threaded web server is constructed by POSIX threads in C language to realize the synchronization.
		This web server is able to handle any file type including HTML, GIF, JPEG, TXT, etc. and of any arbitrary size.
		It is composed of two different types of threads: worker threads and dispatcher threads.
		After the web server is setting up, dispatcher threads will repeatedly accept an incoming connection, read the client request from the connection, and place the request in a queue.
		Worker threads will monitor the request queue, retrieve requests and serve the request’s result back to the client.
		To improve runtime performance, we implement LFU cache replacement policy.

##III. Explanation of caching mechanism used.
		We implemented LFU cache replacement policy. In our cache_entry_t structure, we added a member "hit_times" to represent the frequency of each cache_entry.
		When the cache array is full, we iterate the cache array to find the index of the cache_entry with least frequency, free that cache_entry and that fill that spot with a new cache_entry.
		If all the old entries are with same frequency then remove the cache_entry with the largest index of cache array.
