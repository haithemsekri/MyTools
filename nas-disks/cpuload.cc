#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include  <locale.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

const int NUM_CPU_STATES = 10;

enum CPUStates
{
	S_USER = 0,
	S_NICE,
	S_SYSTEM,
	S_IDLE,
	S_IOWAIT,
	S_IRQ,
	S_SOFTIRQ,
	S_STEAL,
	S_GUEST,
	S_GUEST_NICE
};

typedef struct CPUData
{
	std::string cpu;
	size_t times[NUM_CPU_STATES];
} CPUData;

void ReadStatsCPU(std::vector<CPUData> & entries)
{
	std::ifstream fileStat("/proc/stat");

	std::string line;

	const std::string STR_CPU("cpu");
	const std::size_t LEN_STR_CPU = STR_CPU.size();
	const std::string STR_TOT("tot");

	while(std::getline(fileStat, line))
	{
		// cpu stats line found
		if(!line.compare(0, LEN_STR_CPU, STR_CPU))
		{
			std::istringstream ss(line);

			// store entry
			entries.emplace_back(CPUData());
			CPUData & entry = entries.back();

			// read cpu label
			ss >> entry.cpu;

			// remove "cpu" from the label when it's a processor number
			if(entry.cpu.size() > LEN_STR_CPU)
				entry.cpu.erase(0, LEN_STR_CPU);
			// replace "cpu" with "tot" when it's total values
			else
				entry.cpu = STR_TOT;

			// read times
			for(int i = 0; i < NUM_CPU_STATES; ++i)
				ss >> entry.times[i];
		}
	}
}

size_t GetIdleTime(const CPUData & e)
{
	return	e.times[S_IDLE] + 
			e.times[S_IOWAIT];
}

size_t GetActiveTime(const CPUData & e)
{
	return	e.times[S_USER] +
			e.times[S_NICE] +
			e.times[S_SYSTEM] +
			e.times[S_IRQ] +
			e.times[S_SOFTIRQ] +
			e.times[S_STEAL] +
			e.times[S_GUEST] +
			e.times[S_GUEST_NICE];
}

float GetCpuUsage(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{

  size_t i = 0;
	{
		const CPUData & e1 = entries1[i];
		const CPUData & e2 = entries2[i];

		const float ACTIVE_TIME	= static_cast<float>(GetActiveTime(e2) - GetActiveTime(e1));
		const float IDLE_TIME	= static_cast<float>(GetIdleTime(e2) - GetIdleTime(e1));
		const float TOTAL_TIME	= ACTIVE_TIME + IDLE_TIME;

		return  (100.f * IDLE_TIME / TOTAL_TIME);
	}
}


size_t getnetload(void)
{
  FILE * fp = fopen ("/proc/net/dev", "r");
  if (!fp) {
    return 0;
  } else {
    char buf[1024];
    char * start;
    
    size_t npackets=0;
    
    while((start = fgets(buf, sizeof(buf) - 1, fp))) {
      buf[sizeof(buf) - 1] = 0;
      char * token = strtok(start, " ");
      unsigned k = 0;
      while(token && (token = strtok(NULL, " "))) {
        if(isdigit(*token)) {
          k++;
          if (k == 1 || k == 9) {
            size_t b = strtoul(token, NULL, 10);
            npackets += b;
          }
        }
      }
    }
    
    fclose(fp);
    return npackets;
  }
  return 0;
}

int main(int argc, char * argv[])
{ 
  assert(argc == 4);
  assert(isdigit(*argv[1]));
  
  assert(isdigit(*argv[2]));
  
  assert(isdigit(*argv[3]));
  
  int k = strtoul(argv[3], NULL, 10);
  
  while(true) {
    std::vector<CPUData> entries1;
    size_t netload1 = getnetload ();
    ReadStatsCPU(entries1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    std::vector<CPUData> entries2;
    size_t netload2 = getnetload ();
    ReadStatsCPU(entries2);
    
    float cpustatus = GetCpuUsage(entries1, entries2);
    size_t netload = netload2 - netload1;
    
    std::cout << k << " " << cpustatus << " " << netload << std::endl;
    
    if((netload <= strtoul(argv[2], NULL, 10)) && (cpustatus >= strtoul(argv[1], NULL, 10))) {
      k--;
      if(k < 0) return 0;
    } else {
      k = atoi(argv[3]);
    }
  }
	return 0;
}
