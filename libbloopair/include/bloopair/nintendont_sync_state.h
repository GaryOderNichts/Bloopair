#pragma once

#include <stdint.h>

typedef struct {
    uint32_t processed_generation;
    uint32_t pending_generation;
    uint32_t retry_milliseconds;
} NintendontSyncState;

static inline void NintendontSyncStateInit(NintendontSyncState* state,
                                            uint32_t initial_generation,
                                            uint32_t initial_retry)
{
    state->processed_generation = 0;
    state->pending_generation = initial_generation;
    state->retry_milliseconds = initial_retry;
}

static inline void NintendontSyncStateObserve(NintendontSyncState* state,
                                               uint32_t generation)
{
    state->pending_generation = generation;
}

static inline void NintendontSyncStateCommitted(NintendontSyncState* state,
                                                 uint32_t generation,
                                                 uint32_t initial_retry)
{
    state->processed_generation = generation;
    state->pending_generation = generation;
    state->retry_milliseconds = initial_retry;
}

static inline void NintendontSyncStateFailed(NintendontSyncState* state,
                                              uint32_t maximum_retry)
{
    if (state->retry_milliseconds < maximum_retry) {
        uint32_t doubled = state->retry_milliseconds * 2;
        state->retry_milliseconds = doubled > maximum_retry ? maximum_retry : doubled;
    }
}

static inline int NintendontSyncStatePending(const NintendontSyncState* state)
{
    return state->pending_generation != state->processed_generation;
}
