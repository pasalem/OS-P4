

void memoryMaxer(){

	vAddr indexes[1000];
	for (int i = 0; i < 1000; ++i) {

		indexes[i] = allocateNewInt();
		int *value = accessIntPtr(indexes[i]);
		*value = (i * 3);
		unlockMemory(indexes[i]);

	}

	for (int i = 0; i < 1000; ++i) {
		freeMemory(indexes[i]);
	}
}