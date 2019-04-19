#pragma once

#ifdef _WIN32
#	include <time.h>
#	define _CLOCK_ clock
#else
#	include <sys/time.h> // wall-clock timer (as opposed to clock cycle timer)
/** In Linux, clock() returns CPU cycles, not wall-clock time. */
inline time_t platform_Clock_NIX () {
	/*
	static timeval g_startTime = { 0, 0 };
	if (g_startTime.tv_sec == 0 && g_startTime.tv_usec == 0)
		gettimeofday(&g_startTime, NULL);	// start the timer
	timeval now;
	time_t seconds, useconds, ms;
	gettimeofday(&now, NULL);
	seconds = now.tv_sec - g_startTime.tv_sec;
	useconds = now.tv_usec - g_startTime.tv_usec;
	ms = (seconds * 1000) + (useconds / 1000);
	return (time_t)ms;
	*/
	timeval now;
	gettimeofday (&now, NULL);
	return now.tv_usec;
}

#	define _CLOCK_ platform_Clock_NIX
#endif

inline unsigned char platform_randomBit () {
	static long loops1st, loops2nd;
	static time_t now = _CLOCK_ ();
	for (now = _CLOCK_ (), loops1st = 0; _CLOCK_ () == now; ++loops1st)
		;
	for (now = _CLOCK_ (), loops2nd = 0; _CLOCK_ () == now; ++loops2nd)
		;
	return ((int)(loops2nd < loops1st));
}

/**
* @return relatively random bit sequence. Checks variation in realtime CPU usage.
* Use sparingly!
*/
inline unsigned long platform_randomIntTRNG (int a_numBits) {
	// static reduces function overhead & keeps randomness from the last call
	static unsigned long result = 0, loops1st, loops2nd, index = 0, repetitions = 0;
	static char resulta, resultb;
	static time_t now = _CLOCK_ ();
	for (index = 0; index < a_numBits; ++index) {
		resulta = platform_randomBit ();
		// von Neumann's "fair results from a biased coin" algorithm
		//Toss the coin twice.
		resultb = platform_randomBit ();
		//If the results match, start over, forgetting both results.
		if (resulta == resultb) {
			--index;
			++repetitions;
		} else
			//If the results differ, use the first result, forgetting the second.
			result ^= ((resulta & 1) ^ (repetitions & 1)) << index;
	}
	return result;
}

#undef _CLOCK_

inline unsigned long* __RandomSeed () {
	static unsigned long g_seed = 5223;
	return &g_seed;
}

inline void platform_randomSeed (unsigned long a_seed) { *__RandomSeed () = a_seed; }

inline unsigned long platform_random () {
	// Take the current seed and generate a new value
	// from it. Due to our use of large constants and
	// overflow, it would be very hard for someone to
	// predict what the next number is going to be from
	// the previous one.
	//g_seed = (8253729 * g_seed + 2396403);
	platform_randomSeed (8253729 * (*__RandomSeed ()) + 2396403);

	// return a value between 0 and 2.14 billion (+2^^31)
	return (*__RandomSeed ()) & 0x7fffffff;
}

inline int platform_randomInt () { return (int)platform_random (); }

inline float platform_randomFloat () {
	unsigned long i = platform_random () & 0xffffff;
	return i / (float)(0x1000000);
}

inline float platform_randomFloat (float min, float max) {
	float delta = max - min;
	float number = platform_randomFloat () * delta;
	number += min;
	return number;
}

template<typename TYPE>
inline void platform_shuffle (TYPE* data, const int start, const int limit) {
	int delta = limit - start;
	for (int i = limit - 1; i >= start; --i) {
		int j = (platform_randomInt () % delta) + start;
		TYPE temp = data[i];
		data[i] = data[j];
		data[j] = temp;
	}
}