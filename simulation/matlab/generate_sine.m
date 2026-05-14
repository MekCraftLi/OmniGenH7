function samples = generate_sine(count, amplitude, offset)
% Generate sine wave using 64-entry quarter-wave LUT (same as C++ firmware).
persistent sine_table
if isempty(sine_table)
    sine_table = int16([ ...
        0,  804,  1608,  2411,  3212,  4012,  4808,  5602, ...
        6393,  7180,  7962,  8739,  9512, 10279, 11039, 11793, ...
        12540, 13279, 14010, 14733, 15447, 16151, 16846, 17531, ...
        18205, 18868, 19520, 20160, 20788, 21403, 22006, 22595, ...
        23170, 23732, 24279, 24812, 25330, 25833, 26320, 26791, ...
        27246, 27684, 28106, 28511, 28899, 29269, 29622, 29957, ...
        30274, 30572, 30853, 31114, 31357, 31581, 31786, 31972, ...
        32138, 32286, 32413, 32522, 32610, 32679, 32729, 32758  ...
    ]);
end

samples = zeros(1, count, 'uint16');
for i = 1:count
    phase = floor((i - 1) * 256 / count);
    phase = mod(phase, 256);

    if phase < 64
        val = sine_table(phase + 1);
    elseif phase < 128
        val = sine_table(127 - phase + 1);
    elseif phase < 192
        val = -sine_table(phase - 128 + 1);
    else
        val = -sine_table(255 - phase + 1);
    end

    sample = int32(offset) + int32(val) * int32(amplitude) / 32768;
    if sample < 0, sample = 0; end
    if sample > 4095, sample = 4095; end
    samples(i) = uint16(sample);
end
end
