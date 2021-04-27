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


size_t get_net_packets(void)
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
    static size_t s_npackets = 0;
    size_t npackets_r = npackets - s_npackets;
    s_npackets = npackets;
    //printf("net: %lu ms\n", npackets_r);
    return npackets_r;
  }
  return 0;
}

size_t get_disks_active_time(void)
{
  FILE * fp = fopen ("/proc/diskstats", "r");
  if (!fp) {
    return 0;
  } else {
    char buf[1024];
    char * start;
    
    size_t active_ms = 0;
    
    while((start = fgets(buf, sizeof(buf) - 1, fp))) {
      buf[sizeof(buf) - 1] = 0;
      char * tokens[24];
      char * token = strtok(start, " ");
      unsigned nTokens = 0;
      while(token && (nTokens < (sizeof(tokens) / sizeof(tokens[0])))) {
        tokens[nTokens++] = token;
        token = strtok(NULL, " ");
      }
      if(nTokens < 14) {
        continue;
      }
      if(!isalpha(*tokens[2])) {
        continue;
      }
      if(strstr(tokens[2], "loop")) {
        continue;
      }
      active_ms += strtoul(tokens[13], NULL, 10);
    }
    
    fclose(fp);
    
    static size_t s_active_time = 0;
    size_t active_time_r = active_ms - s_active_time;
    s_active_time = active_ms;
    //printf("disk: %lu ms\n", active_time_r);
    return active_time_r;
  }
  return 0;
}


float get_cpu_idle(void)
{
  FILE * fp = fopen ("/proc/stat", "r");
  if (!fp) {
    return 0;
  } else {
    char buf[1024];
    char * start;
    
    ssize_t active_time = 0;
    ssize_t idle_time = 0;
    
    while((start = fgets(buf, sizeof(buf) - 1, fp))) {
      buf[sizeof(buf) - 1] = 0;
      if(start != strstr(start, "cpu")) {
        break;
      }
      
      char * tokens[20];
      char * token = strtok(start, " ");
      unsigned nTokens = 0;
      while(token && (nTokens < (sizeof(tokens) / sizeof(tokens[0])))) {
        tokens[nTokens++] = token;
        if(nTokens > 11) {
          break;
        }
        token = strtok(NULL, " ");
      }
      if(nTokens > 11) {
        continue;
      }
      
      //printf("%lu %lu %lu %lu %lu / %lu %lu\n", strtoul(tokens[1], NULL, 10),  strtoul(tokens[2], NULL, 10),  strtoul(tokens[3], NULL, 10), strtoul(tokens[6], NULL, 10), strtoul(tokens[7], NULL, 10), strtoul(tokens[4], NULL, 10),  strtoul(tokens[5], NULL, 10));
      active_time += strtoul(tokens[1], NULL, 10) +  strtoul(tokens[2], NULL, 10) +  strtoul(tokens[3], NULL, 10) +  strtoul(tokens[6], NULL, 10) +  strtoul(tokens[7], NULL, 10);
      idle_time += strtoul(tokens[4], NULL, 10) +  strtoul(tokens[5], NULL, 10);
      break;
    }
    
    fclose(fp);
    
    static ssize_t s_active_time = 0;
    static ssize_t s_idle_time = 0;
    size_t active_time1;
    size_t idle_time1;
    
    if(!s_active_time || !s_idle_time) {
      active_time1 = s_active_time = active_time;
      idle_time1 = s_idle_time = idle_time;
    } else {
      active_time1 = (s_active_time > active_time) ? (s_active_time - active_time) :  (active_time - s_active_time);
      idle_time1 = (s_idle_time > idle_time) ? (s_idle_time - idle_time) : (idle_time - s_idle_time);
      s_active_time = active_time;
      s_idle_time = idle_time;
    }
    
    //printf("cpu: %f\n", 100.f * idle_time1 /  (idle_time1 + active_time1));
    
    return 100.f * idle_time1 /  (idle_time1 + active_time1);
  }
  
  return 0;
}

void assert_(bool cond, const char * cond_s)
{
  if(!cond) {
    const char * usage_msg = 
    "\n\targ1::int::cpu_min_precent\n\targ2::int::network_bytes_sec\n\targ3::int::disk_max_percent\n\targ4::int::sample_sec\n\targ5::int::idle_time_sec";
    printf("%s: Invalid argument:%s\n", cond_s, usage_msg);
    exit(1);
  }
}

#define ASSERT_ARGS(cond) assert_(cond, #cond)

#include <syslog.h>

int main(int argc, char * argv[])
{ 
  ASSERT_ARGS(argc == 6);
  ASSERT_ARGS(isdigit(*argv[1]));
  
  ASSERT_ARGS(isdigit(*argv[2]));
  
  ASSERT_ARGS(isdigit(*argv[3]));
  
  ASSERT_ARGS(isdigit(*argv[4]));
  
  ASSERT_ARGS(isdigit(*argv[5]));
  
  unsigned cpu_min_precent = strtoul(argv[1], NULL, 10);
  unsigned network_bytes_sec = strtoul(argv[2], NULL, 10);
  unsigned disk_max_percent = strtoul(argv[3], NULL, 10);
  unsigned sample_sec = strtoul(argv[4], NULL, 10);
  ASSERT_ARGS(sample_sec);
  int idle_time_sec = strtoul(argv[5], NULL, 10);
  

  unsigned disk_active_ms_max = disk_max_percent * sample_sec * 10;
  unsigned net_packet_max =  network_bytes_sec * sample_sec;
  
  float cpu_idle = get_cpu_idle();
  size_t disk_active_ms = get_disks_active_time();
  size_t net_packet = get_net_packets();
  
  openlog (argv[0], LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  while(1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sample_sec * 1000));
    float cpu_idle = get_cpu_idle();
    size_t disk_active_ms = get_disks_active_time();
    size_t net_packet = get_net_packets();
    syslog (LOG_INFO, "%u, cpu: %f/%d , disk: %lu/%u ms , net: %lu/%u Bytes\n", idle_time_sec, cpu_idle, cpu_min_precent, disk_active_ms, disk_active_ms_max, net_packet, net_packet_max);
    if((net_packet <= net_packet_max) && (cpu_idle >= cpu_min_precent) && (disk_active_ms <= disk_active_ms_max)) {
      idle_time_sec-=sample_sec;
      if(idle_time_sec < 1) {
        break;
      }
    } else {
      idle_time_sec = strtoul(argv[5], NULL, 10);
    }    
  }
  syslog (LOG_INFO, "system is idle");
  closelog ();
	return 0;
}
