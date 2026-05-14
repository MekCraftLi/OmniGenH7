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
