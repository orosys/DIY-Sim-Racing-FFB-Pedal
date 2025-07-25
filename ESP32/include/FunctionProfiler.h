#pragma once
#include <Arduino.h>
#include <limits>
#include <string>

// Example usage
// 1) create a profiler instance
// #include "FunctionProfiler.h"
// FunctionProfiler profiler_pedalUpdateTask;
// 2) activate the profiler
// profiler_pedalUpdateTask.activate( true );
// 3) start the timer for ID 0
// profiler_pedalUpdateTask.start(0);
// 4) end the timer for ID 0
// profiler_pedalUpdateTask.end(0);
// 5) print the report
// profiler_pedalUpdateTask.report();




class FunctionProfiler {
public:
    static const int MAX_TIMERS = 16;
    string taskName = "default";
    uint32_t nmbCalls_u32 = 3000;

    void setName(string name)
    {
        taskName = name;
    }

    void setNumberOfCalls(uint32_t nmbCallsArg_u32)
    {
        nmbCalls_u32 = constrain( nmbCallsArg_u32, 1, 10000);
    }

    void activate(bool activeFlagArg_b)
    {
        activeFlag_b = activeFlagArg_b;
    }

    void start(uint8_t id) {
        if (activeFlag_b)
        {
            if (id >= MAX_TIMERS) return;
            startTimes[id] = micros();
            active[id] = true;
        }
        
    }

    void end(uint8_t id) {
        if (activeFlag_b)
        {
            if (id >= MAX_TIMERS || !active[id]) return;
            unsigned long dur = micros() - startTimes[id];
            durations[id] += dur;
            counts[id]++;
            last[id] = dur;
            if (dur < mins[id]) mins[id] = dur;
            if (dur > maxs[id]) maxs[id] = dur;
            active[id] = false;
        }
    }

    void reset() {
        for (int i = 0; i < MAX_TIMERS; ++i) {
            durations[i] = 0;
            counts[i] = 0;
            mins[i] = std::numeric_limits<unsigned long>::max();
            maxs[i] = 0;
            last[i] = 0;
            active[i] = false;
        }
    }

    void report() {
        if (activeFlag_b)
            {
            if (counts[0] >= nmbCalls_u32)
            {
                // Serial.println(F("\n------ FunctionProfiler Report (by ID) for task: %s------"));
                Serial.printf("\n------ FunctionProfiler Report (by ID) for task: %s ------\n", taskName.c_str());
                // Serial.printf("%s\n", taskName.c_str());
                for (int i = 0; i < MAX_TIMERS; ++i) {
                    if (counts[i] > 0) {
                        unsigned long avg = durations[i] / counts[i];
                        Serial.printf("ID %2d: calls=%lu | avg=%lu us | min=%lu us | max=%lu us | last=%lu us\n",
                            i, counts[i], avg, mins[i], maxs[i], last[i]);
                    }
                }
                Serial.println(F("---------------------------------------------"));

                // reset the values
                reset();
            }
        }
        
    }

    

private:
    unsigned long startTimes[MAX_TIMERS] = {0};
    unsigned long durations[MAX_TIMERS] = {0};
    unsigned long last[MAX_TIMERS] = {0};
    unsigned long mins[MAX_TIMERS];
    unsigned long maxs[MAX_TIMERS] = {0};
    uint32_t counts[MAX_TIMERS] = {0};
    bool active[MAX_TIMERS] = {false};
    bool activeFlag_b = false;

public:
    FunctionProfiler() {
        for (int i = 0; i < MAX_TIMERS; ++i) {
            mins[i] = std::numeric_limits<unsigned long>::max();
        }
    }
};
