#include <bloopair/nintendont_sync_state.h>

#include <cassert>

int main()
{
    NintendontSyncState state{};
    NintendontSyncStateInit(&state, 4, 500);
    assert(NintendontSyncStatePending(&state));

    NintendontSyncStateFailed(&state, 8000);
    assert(state.processed_generation == 0);
    assert(state.pending_generation == 4);
    assert(state.retry_milliseconds == 1000);

    NintendontSyncStateObserve(&state, 5);
    NintendontSyncStateFailed(&state, 8000);
    assert(state.pending_generation == 5);
    assert(state.processed_generation == 0);

    NintendontSyncStateCommitted(&state, 5, 500);
    assert(!NintendontSyncStatePending(&state));
    assert(state.retry_milliseconds == 500);

    NintendontSyncStateObserve(&state, 6);
    for (int i = 0; i < 8; i++) NintendontSyncStateFailed(&state, 8000);
    assert(state.retry_milliseconds == 8000);
    assert(NintendontSyncStatePending(&state));
}
