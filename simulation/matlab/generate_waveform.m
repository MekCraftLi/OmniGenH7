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
