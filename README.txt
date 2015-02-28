README - PROJECT 4
ALEC BENSON
PETER SALEM


to compile, type "make".
to run, type ./api #

where # is:
	0 for LRU eviction
	1 for random eviction

To switch between single threading and multi threading:

scroll down to the main function():

	pthread_t thread1, thread2, thread3;
	pthread_create(&thread1, NULL, &thrash, NULL);
	pthread_create(&thread2, NULL, &thrash, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	//thrash();
	//timing_test();

Multi threading is the default. 
You may adjust the test that each thread runs by changing the pointer in the pthread_create method.
If you want to run in a single thread, comment out the pthread_create and pthread join methods, and simply call the appropriate test at the bottom.

You can change the access time delays in the delay() function. 
