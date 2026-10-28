# v00.21: immediate recovery after native audio underruns

Hardware feedback on v020: sound improved, but a delay was heard roughly every
half second. No v020 session log has yet established whether periodic FIFO
starvation, synthesis/render timing or another fault causes that pattern.

The backend had an independently reproducible recovery problem: every partial
or empty output grain set playback back to its startup prefill state. A small
shortage could therefore add a 64–80ms refill wait before sound resumed.

Keep the 3072-frame/80ms prefill only at startup. Once playback has started,
continue submitting native blocks, zero-fill unavailable samples, and consume
new PCM as soon as the next block can take it. Empty gaps arm the existing
fade-in to avoid a hard transition on recovery. Sample rate, synthesis, queue
capacity, guest DMA timing, native priority and rendering are unchanged.
This removes the deliberate recovery wait; it cannot fix sustained insufficient
sample production, and actual pauses may remain shorter gaps or crackles.

Regression: drain one short DMA, observe an underrun, submit a new short DMA
without advancing the fake process clock, and require audible PCM to resume.
The test fails on unchanged v020's backend and passes on v021. Existing AI,
resampler, priority/fallback, failure/shutdown, gap and concurrent FIFO tests
also pass. Full VitaSDK compile/link performed; hardware quality remains
unverified for this candidate.

Install v021 over DUCKVIT04, compare the same gameplay scene with v020, and
send `ux0:data/DuckRecomp/game.log` if the regular delay remains. Keep v020
available for rollback. ROM/save paths and the Select settings menu are kept.
