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
