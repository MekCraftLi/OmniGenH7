%% waveform_generator.m
% Waveform synthesis functions matching OmniGenH7 firmware algorithms.
% All functions output 12-bit DAC values (0-4095), center = 2048.

function samples = generate_sine(count, amplitude, offset)
    % Generate sine wave using 64-entry quarter-wave LUT (same as C++).
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
        phase = floor((i - 1) * 256 / count);  % 0..255
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

function samples = generate_square(count, amplitude, offset, duty)
    % Square wave with duty cycle in permille (0-1000).
    threshold = floor(count * duty / 1000);

    high_level = int32(offset) + int32(amplitude) / 2;
    low_level  = int32(offset) - int32(amplitude) / 2;
    if high_level > 4095, high_level = 4095; end
    if low_level < 0,    low_level = 0;     end

    samples = zeros(1, count, 'uint16');
    for i = 1:count
        if i <= threshold
            samples(i) = uint16(high_level);
        else
            samples(i) = uint16(low_level);
        end
    end
end

function samples = generate_triangle(count, amplitude, offset)
    % Symmetric triangle wave.
    min_val = int32(offset) - int32(amplitude) / 2;
    max_val = int32(offset) + int32(amplitude) / 2;
    if min_val < 0,    min_val = 0;     end
    if max_val > 4095, max_val = 4095;  end

    samples = zeros(1, count, 'uint16');
    for i = 1:count
        phase = floor(2 * (i - 1) * (max_val - min_val) / count) + min_val;
        if phase > max_val
            phase = 2 * max_val - phase;
        end
        samples(i) = uint16(phase);
    end
end

function samples = generate_sawtooth(count, amplitude, offset)
    % Linear ramp from min to max.
    min_val = int32(offset) - int32(amplitude) / 2;
    max_val = int32(offset) + int32(amplitude) / 2;
    if min_val < 0,    min_val = 0;     end
    if max_val > 4095, max_val = 4095;  end

    samples = zeros(1, count, 'uint16');
    for i = 1:count
        sample = min_val + floor((i - 1) * (max_val - min_val) / count);
        samples(i) = uint16(sample);
    end
end

function samples = generate_waveform(count, profile)
    % Generate waveform based on signal profile struct.
    amplitude_dac = voltage_to_dac(profile.amplitude_mv) / 2;
    offset_dac    = voltage_to_dac(profile.offset_mv);

    switch profile.kind
        case 'sine'
            samples = generate_sine(count, amplitude_dac, offset_dac);
        case 'square'
            samples = generate_square(count, amplitude_dac, offset_dac, profile.duty);
        case 'triangle'
            samples = generate_triangle(count, amplitude_dac, offset_dac);
        case 'sawtooth'
            samples = generate_sawtooth(count, amplitude_dac, offset_dac);
        otherwise
            samples = generate_sine(count, amplitude_dac, offset_dac);
    end
end

function dac_val = voltage_to_dac(voltage_mv, vref_mv)
    % Convert mV to 12-bit DAC value (same as C++ firmware).
    if nargin < 2, vref_mv = 3300; end
    if voltage_mv < 0
        dac_val = 0;
    elseif voltage_mv >= vref_mv
        dac_val = 4095;
    else
        dac_val = uint16(floor(voltage_mv * 4095 / vref_mv));
    end
end
