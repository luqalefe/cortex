declare name "Codex Delay";
declare author "Codex";
import("stdfaust.lib");

time    = hslider("time[unit:ms]", 480, 1, 2000, 1) : si.smoo;
repeats = hslider("repeats", 0.4, 0, 0.95, 0.01) : si.smoo;
mix     = hslider("mix", 0.35, 0, 1, 0.01) : si.smoo;
tone    = hslider("tone[unit:Hz]", 6000, 800, 16000, 1) : si.smoo;

maxd  = 262144;                          // ~2,7 s a 96 kHz (potência de 2)
d     = ba.sec2samp(time / 1000);
wet   = (+ : de.fdelay(maxd, d)) ~ (*(repeats) : fi.lowpass(2, tone));
process = _ <: *(1 - mix), (wet : *(mix)) :> _;
